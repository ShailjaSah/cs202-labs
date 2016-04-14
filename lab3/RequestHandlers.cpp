#include "RequestHandlers.h"
/*
 * ------------------------------------------------------------------
 * add_item_handler --
 *
 *      Handle an AddItemReq.
 *
 *      Delete the request object when done.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void 
add_item_handler(void *args)
{
  AddItemReq* rq = (AddItemReq*)args;
  printf("Handling AddItemReq:item_id: %d quantity: %d rq->price: %f rq->discount: %f \n", rq->item_id, rq->quantity, rq->price, rq->discount);
  fflush(stdout);
  EStore* e = rq->store;
  e->addItem(rq->item_id, rq->quantity, rq->price, rq->discount);
  delete rq;
}

/*
 * ------------------------------------------------------------------
 * remove_item_handler --
 *
 *      Handle a RemoveItemReq.
 *
 *      Delete the request object when done.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void 
remove_item_handler(void *args)
{
  RemoveItemReq* rq = (RemoveItemReq*) args;
  printf("Handling RemoveItemReq :item_id: %d\n", rq->item_id);
  fflush(stdout);
  EStore* es = rq->store;
  es->removeItem(rq->item_id);
  delete rq;
}

/*
 * ------------------------------------------------------------------
 * add_stock_handler --
 *
 *      Handle an AddStockReq.
 *
 *      Delete the request object when done.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void 
add_stock_handler(void *args)
{
  AddStockReq *rq = (AddStockReq *) args;
  printf("Handling AddStockReq:item_id: %d additional_stock: %d\n", rq->item_id, rq->additional_stock);
  fflush(stdout);
  EStore* es = rq->store;
  es->addStock(rq->item_id, rq->additional_stock);
  delete rq;
}

/*
 * ------------------------------------------------------------------
 * change_item_price_handler --
 *
 *      Handle a ChangeItemPriceReq.
 *
 *      Delete the request object when done.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void 
change_item_price_handler(void *args)
{
  ChangeItemPriceReq* rq = (ChangeItemPriceReq *) args;
  printf("Handling ChangeItemPriceReq:item_id: %d new_price:%f\n", rq->item_id, rq->new_price);
  fflush(stdout);
  EStore* es = rq->store;
  es->priceItem(rq->item_id, rq->new_price);
  delete rq;
}

/*
 * ------------------------------------------------------------------
 * change_item_discount_handler --
 *
 *      Handle a ChangeItemDiscountReq.
 *
 *      Delete the request object when done.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void 
change_item_discount_handler(void *args)
{
  ChangeItemDiscountReq* rq = (ChangeItemDiscountReq *) args;
  printf("Handling ChangeItemDiscountReq:item_id: %d new_discount: %f\n", rq->item_id, rq->new_discount);
  fflush(stdout);
  EStore* es = rq->store;
  es->discountItem(rq->item_id, rq->new_discount);
  delete rq;
}

/*
 * ------------------------------------------------------------------
 * set_shipping_cost_handler --
 *
 *      Handle a SetShippingCostReq.
 *
 *      Delete the request object when done.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void 
set_shipping_cost_handler(void *args)
{
  SetShippingCostReq* rq = (SetShippingCostReq* ) args;
  printf("Handling SetShippingCostReq: new_cost %f\n", rq->new_cost);
  fflush(stdout);
  EStore* es = rq->store;
  es->setShippingCost(rq->new_cost);
  delete rq;
}

/*
 * ------------------------------------------------------------------
 * set_store_discount_handler --
 *
 *      Handle a SetStoreDiscountReq.
 *
 *      Delete the request object when done.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void
set_store_discount_handler(void *args)
{
  SetStoreDiscountReq* rq = (SetStoreDiscountReq *) args;
  printf("Handling SetStoreDiscountReq: new_discount: %f\n", rq->new_discount);
  fflush(stdout);
  EStore* es = rq->store;
  es->setStoreDiscount(rq->new_discount);
  delete rq;
}

/*
 * ------------------------------------------------------------------
 * buy_item_handler --
 *
 *      Handle a BuyItemReq.
 *
 *      Delete the request object when done.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void
buy_item_handler(void *args)
{
  BuyItemReq* rq = (BuyItemReq *) args;
  printf("Handling BuyItemReq:item_id: %d, budget: %f\n", rq->item_id, rq->budget);
  fflush(stdout);
  EStore* es = rq->store;
  es->buyItem(rq->item_id, rq->budget);
  delete rq;
}

/*
 * ------------------------------------------------------------------
 * buy_many_items_handler --
 *
 *      Handle a BuyManyItemsReq.
 *
 *      Delete the request object when done.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void
buy_many_items_handler(void *args)
{
  BuyManyItemsReq* rq = (BuyManyItemsReq *) args;
  printf("Handing BuyManyItemsReq : item_id: ");
  for (size_t i = 0; i < rq->item_ids.size(); i++){
    printf("%d ", rq->item_ids[i]);
  }
  printf("\n");
  fflush(stdout);
  EStore * es = rq->store;
  es->buyManyItems(&rq->item_ids, rq->budget);
  delete rq;
}

/*
 * ------------------------------------------------------------------
 * stop_handler --
 *
 *      The thread should exit.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void 
stop_handler(void* args)
{
  fflush(stdout);
  printf("Thread Exit\n");
  sthread_exit();
}

