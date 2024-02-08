#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include <linux/bio.h>
#include <linux/device-mapper.h>

struct linear_c {
        struct dm_dev *dev;
        sector_t start;
};

static int linear_target_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
        struct linear_c *mdt;
        unsigned long long tmp;
        char dummy;
        int ret;

        printk(KERN_CRIT "in function linear_target_ctr");

        if (argc != 2) {
                printk(KERN_CRIT "Invalid no.of arguments.");
                ti->error = "Invalid argument count";
                return -EINVAL;
        }

        mdt = kmalloc(sizeof(*mdt), GFP_KERNEL);
        if(mdt == NULL)
        {
                printk(KERN_CRIT "Mdt is null");
                ti->error = "dm-linear_target: Cannot allocate linear context";
                return -ENOMEM;
        }

        ret = -EINVAL;
        if (sscanf(argv[1], "%llu%c", &tmp, &dummy) != 1 || tmp != (sector_t)tmp) {
                ti->error = "Invalid device sector";
                goto bad;
        }
        mdt->start = tmp;

        ret = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &mdt->dev);
        if (ret) {
                ti->error = "dm-linear_target: Device lookup failed";
                goto bad;
        }

        ti->num_flush_bios = 1;
        ti->num_discard_bios = 1;
        ti->num_secure_erase_bios = 1;
        ti->num_write_zeroes_bios = 1;
        ti->private = mdt;


        printk(KERN_CRIT "out function linear_target_ctr");
        return 0;

        bad:
        kfree(mdt);
        printk(KERN_CRIT "out function linear_target_ctr with err");
        return ret;
}

static void linear_target_dtr(struct dm_target *ti)
{
        struct linear_c *mdt = ti->private;
        printk(KERN_CRIT "in function linear_target_dtr");
        dm_put_device(ti, mdt->dev);
        kfree(mdt);
        printk(KERN_CRIT "out function linear_target_dtr");
}

static sector_t linear_map_sector(struct dm_target *ti, sector_t bi_sector)
{
        struct linear_c *mdt = ti->private;

        return mdt->start + dm_target_offset(ti, bi_sector);
}

static int linear_target_map(struct dm_target *ti, struct bio *bio)
{
        struct linear_c *mdt = ti->private;

        printk(KERN_CRIT "in function linear_target_map");


        bio_set_dev(bio, mdt->dev->bdev);
        bio->bi_iter.bi_sector = linear_map_sector(ti, bio->bi_iter.bi_sector);

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
                printk(KERN_CRIT "Error in registering target");

        printk(KERN_CRIT "in function init_linear_target");

        return r;
}

static void cleanup_linear_target(void)
{
        dm_unregister_target(&linear_target);

        printk(KERN_CRIT "in function cleanup_linear_target");
}

module_init(init_linear_target);
module_exit(cleanup_linear_target);
MODULE_LICENSE("GPL");