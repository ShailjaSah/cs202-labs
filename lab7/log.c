#include "log.h"
#include "fsdriver.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "passert.h"
#include "inode.h"
#include "disk_map.h"

/*
 * log_tx_add adds a new entry to the log
 * It does so by modifying the fields of log->entries[log->nentries]
 *
 * Hint:
 *      Don't forget to increment log->nentries after you are done
 *      Your code should also handle the case where log_args == NULL;
 *          in that case, you should simply log the op code and time stamp,
 *          and increment the nentries counter
 */
void
log_tx_add(operation_t op, const log_args_t *log_args, time_t time)
{
	struct log *log = s_log;
  struct log_entry *entry = &log->entries[log->nentries++];
  entry->txn_id = log->txn_id;
  entry->op = op;
  entry->time = time;
  if (log_args){
    entry->args = *log_args;
  }
}

/*
 * log_tx_done - sets next available log->entries to be the *commit* entry
 *               and sets corresponding fields to be log->txn_id, OP_COMMIT
 *             - uses msync to persist the entire log. This is *NOT* atomic,
 *               since it's more than one sector size.
 *             - increments the transaction id of the log
 *             - increments the log->nentries, so that the newly appended
 *               *commit* entry will be replayed.
 *             - uses msync to persist the first sector of log. This disk write is done atomically.
 * Return TX_COMMIT once the commit entry has persisted to disk.
 *
 */
int
log_tx_done()
{
	struct log *log = s_log;
  struct log_entry entry;
  entry.txn_id = log->txn_id;
  entry.op = OP_COMMIT;
  log->entries[log->nentries] = entry;
  msync(log, sizeof(struct log), MS_SYNC);
  log->txn_id++;
  log->nentries++;
  msync(log, SECTORSIZE, MS_SYNC);
	return TX_COMMIT;
}

// Abandons the current transaction: increase the log's transaction id
int
log_tx_abandon()
{
	++s_log->txn_id;
        int r;
	if ((r = msync(s_log, SECTORSIZE, MS_SYNC)) < 0) {
		panic("msync: %s", strerror(errno));
	}
	return TX_INVALID;
}

// Takes a single entry and *installs* it onto the disk by calling the correct
// function with the correct arguments.
void
log_entry_install(const struct log_entry* entry) {
	switch (entry->op) {
		case OP_INIT:
			printf("op_init with message: %s\n", entry->args.init_args.msg);
			break;
		case OP_GETATTR:
			break;
		case OP_READLINK:
			{
				const struct readlink_args_t *args = &entry->args.readlink_args;
				fs_readlink_install(args->path, NULL, args->len, entry->time);
				break;
			}
		case OP_MKNOD:
			{
        const struct mknod_args_t *args = &entry->args.mknod_args;
        fs_mknod_install(args->path, args->mode, args->rdev, args->uid, args->gid, entry->time);
				break;
			}
		case OP_OPEN:
			break;
		case OP_READDIR:
			{
				const struct readdir_args_t *args = &entry->args.readdir_args;
				fs_readdir_install(args->path, NULL, NULL, args->offset, args->i_num, entry->time);
				break;
			}
		case OP_UNLINK:
			{
        const struct unlink_args_t *args = &entry->args.unlink_args;
        fs_unlink_install(args->path, entry->time);
				break;
			}
		case OP_RENAME:
			{
        const struct rename_args_t *args = &entry->args.rename_args;
        fs_rename_install(args->srcpath, args->dstpath, entry->time);
				break;
			}
		case OP_LINK:
			{
        const struct link_args_t *args = &entry->args.link_args;
        fs_link_install(args->srcpath, args->dstpath, entry->time);
				break;
			}
		case OP_CHMOD:
			{
				const struct chmod_args_t *args = &entry->args.chmod_args;
				fs_chmod_install(args->path, args->mode, entry->time);
				break;
			}
		case OP_CHOWN:
			{
				const struct chown_args_t *args = &entry->args.chown_args;
				fs_chown_install(args->path, args->uid, args->gid, entry->time);
				break;
			}
		case OP_TRUNCATE:
			{
        const struct truncate_args_t *args = &entry->args.truncate_args;
        fs_truncate_install(args->path, args->size, entry->time);
				break;
			}
		case OP_READ:
			{
				const struct read_args_t *args = &entry->args.read_args;
				fs_read_install(args->path, NULL, args->size, args->offset, args->i_num, entry->time);
				break;
			}
		case OP_WRITE:
			{
        const struct write_args_t *args = &entry->args.write_args;
        fs_write_install(args->path, args->buf, args->size, args->offset, args->i_num, entry->time);
				break;
			}
		case OP_STATFS:
			break;
		case OP_FSYNC:
			break;
		case OP_FGETATTR:
			break;
		case OP_UTIMENS:
			{
				const struct utimens_args_t *args = &entry->args.utimens_args;
				fs_utimens_install(args->path, args->tv, entry->time);
				break;
			}

		case OP_IOCTL:
		case OP_COMMIT:
		case OP_ABORT:
			break;
		default:
			panic("log_entry_install: couldn't install opcode %u\n", entry->op);
	}
}

/*
 * log_replay: Scan through committed transactions and apply their effects
 *      by using log_entry_install.
 *
 * There are several ways to do this.
 *
 *   One way is go through all log entries twice. In the first pass,
 *   find all committed txn_ids.  In the second pass, replay all log
 *   entries with a committed txn_id.  You may need to use malloc() and
 *   free(). If you use malloc(), be sure to include an appropriate
 *   free() call.
 *
 * There is also a single-pass solution; in that solution, it may be helpful to
 *   conceptualize the required logic as a finite state machine.
 *
 */
void
log_replay() {
	struct log *log = s_log;
  uint32_t size = (log->txn_id - log->entries[0].txn_id) * sizeof(uint32_t);
  uint32_t* ids = malloc(size);
  memset(ids, 0, size);
  uint32_t p = 0;
  for (uint32_t i = 0; i < log->nentries; i++){
    if (log->entries[i].op == OP_COMMIT){
      ids[p] = log->entries[i].txn_id;
      p++;
    }
  }

  for (uint32_t i = 0; i < log->nentries; i++){
    uint32_t id = log->entries[i].txn_id;
    for (uint32_t j = 0; j < p; j++){
      if (id == ids[j]){
        log_entry_install(&log->entries[i]);
        break;
      }
    }
  }

  free(ids);
}
