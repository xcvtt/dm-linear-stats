#include "linux/kobject.h"
#include "linux/sysfs.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/device-mapper.h>

#define DM_MSG_PREFIX "linear-stats"

struct linear_c {
        struct dm_dev *dev;
        sector_t start;
};

struct bio_stats {
        int read_count;
        int write_count;
        long read_bytes;
        long write_bytes;
};

static struct bio_stats stats;

static struct kobject *stats_kobj;

static void update_bio_stats(struct bio *bio)
{
        long bytes = bio->bi_iter.bi_size;

        if (op_is_write(bio->bi_opf)) {
                DMINFO("OP_WRITE");
                stats.write_count++;
                stats.write_bytes += bytes;
        } else {
                DMINFO("OP_READ");
                stats.read_count++;
                stats.read_bytes += bytes;
        }
}

static ssize_t stats_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        DMINFO("in stats_show");
        long avg_read = stats.read_count == 0 ? 0 : stats.read_bytes / stats.read_count;
        long avg_write = stats.write_count == 0 ? 0 : stats.write_bytes / stats.write_count;
        int total_req = stats.read_count + stats.write_count;
        long total_avg = total_req == 0 ? 0 : (stats.read_bytes + stats.write_bytes) / total_req;

        return sysfs_emit(buf, 
                          "read req: %d\navg read: %ld\n"
                          "write req: %d\navg write: %ld\n"
                          "total req: %d\navg total: %ld\n",
                          stats.read_count, avg_read,
                          stats.write_count, avg_write,
                          total_req, total_avg);
}

static struct kobj_attribute stats_attr = __ATTR_RO(stats);

static int linear_target_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
        struct linear_c *mdt;
        int ret;

        DMINFO("linear_target_ctr");

        if (argc != 1) {
                ti->error = "Invalid argument count";
                return -EINVAL;
        }

        DMINFO("block device: %s", argv[0]);

        mdt = kmalloc(sizeof(*mdt), GFP_KERNEL);
        if(mdt == NULL)
        {
                ti->error = "dm-linear_target: Cannot allocate linear context";
                return -ENOMEM;
        }

        ret = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &mdt->dev);
        if (ret) {
                ti->error = "dm-linear_target: Device lookup failed";
                goto bad;
        }

        ti->num_discard_bios = 1;
        ti->discards_supported = true;
        ti->private = mdt;

        stats_kobj = kobject_create_and_add("stat", &THIS_MODULE->mkobj.kobj);
        if (stats_kobj == NULL)
                return -ENOMEM;

        ret = sysfs_create_file(stats_kobj, &stats_attr.attr);
        if (ret) {
                DMERR("failed to create sysfs file");
                goto bad;
        }

        DMINFO("linear ctr exit");

        return 0;

        bad:
        kfree(mdt);
        kobject_put(stats_kobj);
        return ret;
}

static void linear_target_dtr(struct dm_target *ti)
{
        struct linear_c *mdt = ti->private;

        dm_put_device(ti, mdt->dev);
        kobject_put(stats_kobj);
        kfree(mdt);
}

static int linear_target_map(struct dm_target *ti, struct bio *bio)
{
        struct linear_c *mdt = ti->private;

        update_bio_stats(bio);
        bio_set_dev(bio, mdt->dev->bdev);

        return DM_MAPIO_REMAPPED;
}

static struct target_type linear_target = {

        .name = "linear-stats",
        .version = {1,0,0},
        .module = THIS_MODULE,
        .ctr = linear_target_ctr,
        .dtr = linear_target_dtr,
        .map = linear_target_map,
};

static int init_linear_target(void)
{
        int r = dm_register_target(&linear_target);

        if (r < 0)
                DMERR("register failed %d", r);

        return r;
}

static void cleanup_linear_target(void)
{
        dm_unregister_target(&linear_target);
}

module_init(init_linear_target);
module_exit(cleanup_linear_target);
MODULE_LICENSE("GPL");
