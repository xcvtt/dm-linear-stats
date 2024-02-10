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

        ti->private = mdt;

        return 0;

        bad:
        kfree(mdt);
        return ret;
}

static void linear_target_dtr(struct dm_target *ti)
{
        struct linear_c *mdt = ti->private;

        dm_put_device(ti, mdt->dev);
        kfree(mdt);
}

static int linear_target_map(struct dm_target *ti, struct bio *bio)
{
        struct linear_c *mdt = ti->private;

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
