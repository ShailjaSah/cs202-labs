#include <cassert>

#include "EStore.h"

using namespace std;


Item::
Item() : valid(false), quantity(0)
{
}

Item::
~Item()
{
}


EStore::
EStore(bool enableFineMode)
    : fineMode(enableFineMode)
{
  smutex_init(&lock);
  scond_init(&available);
  if (fineMode){
    for (int i = 0; i < INVENTORY_SIZE; i++){
      smutex_init(&locks[i]);
    }
  }
}

EStore::
~EStore()
{
  smutex_destroy(&lock);
  scond_destroy(&available);
  if (fineMode){
    for (int i = 0; i < INVENTORY_SIZE; i++){
      smutex_destroy(&locks[i]);
    }
  }
}

/*
 * ------------------------------------------------------------------
 * buyItem --
 *
 *      Attempt to buy the item from the store.
 *
 *      An item can be bought if:
 *          - The store carries it.
 *          - The item is in stock.
 *          - The cost of the item plus the cost of shipping is no
 *            more than the budget.
 *
 *      If the store *does not* carry this item, simply return and
 *      do nothing. Do not attempt to buy the item.
 *
 *      If the store *does* carry the item, but it is not in stock
 *      or its cost is over budget, block until both conditions are
 *      met (at which point the item should be bought) or the store
 *      removes the item from sale (at which point this method
 *      returns).
 *
 *      The overall cost of a purchase for a single item is defined
 *      as the current cost of the item times 1 - the store
 *      discount, plus the flat overall store shipping fee.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
buyItem(int item_id, double budget)
{
    assert(!fineModeEnabled());
    if (item_id < 0 || item_id > INVENTORY_SIZE){
      return;
    }
    smutex_lock(&lock);
    Item item = inventory[item_id];
    while (!item.valid|| item.quantity == 0 || item.price * (1 - item.discount) > budget){
      scond_wait(&available, &lock);
    }
    inventory[item_id].quantity--;
    smutex_unlock(&lock);
}

/*
 * ------------------------------------------------------------------
 * buyManyItem --
 *
 *      Attempt to buy all of the specified items at once. If the
 *      order cannot be bought, give up and return without buying
 *      anything. Otherwise buy the entire order at once.
 *
 *      The entire order can be bought if:
 *          - The store carries all items.
 *          - All items are in stock.
 *          - The cost of the the entire order (cost of items plus
 *            shipping for each item) is no more than the budget.
 *
 *      If multiple customers are attempting to buy at the same
 *      time and their orders are mutually exclusive (i.e., the
 *      two customers are not trying to buy any of the same items),
 *      then their orders must be processed at the same time.
 *
 *      For the purposes of this lab, it is OK for the store
 *      discount and shipping cost to change while an order is being
 *      processed.
 *
 *      The cost of a purchase of many items is the sum of the
 *      costs of purchasing each item individually. The purchase
 *      cost of an individual item is covered above in the
 *      description of buyItem.
 *
 *      Challenge: For bonus points, implement a version of this
 *      method that will wait until the order can be fulfilled
 *      instead of giving up. The implementation should be efficient
 *      in that it should not wake up threads unecessarily. For
 *      instance, if an item decreases in price, only threads that
 *      are waiting to buy an order that includes that item should be
 *      signaled (though all such threads should be signaled).
 *
 *      Challenge: For bonus points, ensure that the shipping cost
 *      and store discount does not change while processing an
 *      order.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
buyManyItems(vector<int>* item_ids, double budget)
{
    assert(fineModeEnabled());
    double sum = 0;
    for (size_t i = 0; i < item_ids->size(); i++){
      int id = (*item_ids)[i];
      smutex_lock(&locks[id]);
      Item item = inventory[id];
      if (!item.valid || !item.quantity){
        smutex_unlock(&locks[id]);
        return;
      }
      double amount = item.price * (1 - item.discount) + shipping_cost;
      sum += amount;
      if (sum > budget){
        smutex_unlock(&locks[id]);
        return;
      }
      smutex_unlock(&locks[id]);
    }

    for (size_t i = 0; i < item_ids->size(); i++){
      int id = (*item_ids)[i];
      smutex_lock(&locks[id]);
      inventory[id].quantity--;
      smutex_unlock(&locks[id]);
    }
}

/*
 * ------------------------------------------------------------------
 * addItem --
 *
 *      Add the item to the store with the specified quantity,
 *      price, and discount. If the store already carries an item
 *      with the specified id, do nothing.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
addItem(int item_id, int quantity, double price, double discount)
{
  
  assert(item_id < INVENTORY_SIZE);
  if (fineMode){
    smutex_lock(&locks[item_id]);
  } else {
    smutex_lock(&lock); 
  }
  Item item = inventory[item_id];
  if (item.valid){
    if (fineMode){
      smutex_unlock(&locks[item_id]);
    } else {
      smutex_unlock(&lock);
    }
    return;
  }
  item.valid = true;
  item.quantity = quantity;
  item.price = price;
  item.discount = discount;
  inventory[item_id] = item;
  scond_broadcast(&available, &lock);
  if (fineMode){
    smutex_unlock(&locks[item_id]);
  } else {
    smutex_unlock(&lock);
  }
}

/*
 * ------------------------------------------------------------------
 * removeItem --
 *
 *      Remove the item from the store. The store no longer carries
 *      this item. If the store is not carrying this item, do
 *      nothing.
 *
 *      Wake any waiters.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
removeItem(int item_id)
{
  assert(item_id < INVENTORY_SIZE);
  if (fineMode){
    smutex_lock(&locks[item_id]);
  } else {
    smutex_lock(&lock); 
  }
  inventory[item_id].valid = false;
  smutex_unlock(&lock);
  if (fineMode){
    smutex_unlock(&locks[item_id]);
  } else {
    smutex_unlock(&lock);
  }

}

/*
 * ------------------------------------------------------------------
 * addStock --
 *
 *      Increase the stock of the specified item by count. If the
 *      store does not carry the item, do nothing. Wake any waiters.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
addStock(int item_id, int count)
{
  assert(item_id < INVENTORY_SIZE);
  if (fineMode){
    smutex_lock(&locks[item_id]);
  } else {
    smutex_lock(&lock); 
  }
  inventory[item_id].quantity += count;
  scond_broadcast(&available, &lock);
  if (fineMode){
    smutex_unlock(&locks[item_id]);
  } else {
    smutex_unlock(&lock);
  }
}

/*
 * ------------------------------------------------------------------
 * priceItem --
 *
 *      Change the price on the item. If the store does not carry
 *      the item, do nothing.
 *
 *      If the item price decreased, wake any waiters.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
priceItem(int item_id, double price)
{
  assert(item_id < INVENTORY_SIZE);
  if (fineMode){
    smutex_lock(&locks[item_id]);
  } else {
    smutex_lock(&lock); 
  }
  inventory[item_id].price = price;
  if (fineMode){
    smutex_unlock(&locks[item_id]);
  } else {
    smutex_unlock(&lock);
  }
}

/*
 * ------------------------------------------------------------------
 * discountItem --
 *
 *      Change the discount on the item. If the store does not carry
 *      the item, do nothing.
 *
 *      If the item discount increased, wake any waiters.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
discountItem(int item_id, double discount)
{
  assert(item_id < INVENTORY_SIZE);
  if (fineMode){
    smutex_lock(&locks[item_id]);
  } else {
    smutex_lock(&lock); 
  }
  inventory[item_id].discount = discount;
  if (fineMode){
    smutex_unlock(&locks[item_id]);
  } else {
    smutex_unlock(&lock);
  }
}

/*
 * ------------------------------------------------------------------
 * setShippingCost --
 *
 *      Set the per-item shipping cost. If the shipping cost
 *      decreased, wake any waiters.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
setShippingCost(double cost)
{
  smutex_lock(&lock);
  bool decresed = cost < shipping_cost;
  shipping_cost = cost;
  if (decresed){
    scond_broadcast(&available, &lock);
  }
  smutex_unlock(&lock);
}

/*
 * ------------------------------------------------------------------
 * setStoreDiscount --
 *
 *      Set the store discount. If the discount increased, wake any
 *      waiters.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
setStoreDiscount(double discount)
{
  smutex_lock(&lock);
  bool increased = discount > store_discount;
  store_discount = discount;
  if (increased){
    scond_broadcast(&available, &lock);
  }
  smutex_unlock(&lock);
}


