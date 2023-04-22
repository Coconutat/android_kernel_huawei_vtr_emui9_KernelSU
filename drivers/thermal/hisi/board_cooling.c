#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/err.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <trace/events/thermal.h>

typedef u32 (*get_power_t)(void);

struct board_cooling_device {
	int id;
	struct thermal_cooling_device *cdev;
	unsigned int max_level;
	unsigned int current_level;
	u32 *power_table;
	struct list_head node;
	get_power_t get_power;
};
/*lint -e708 -e570 -e64 -e785 -esym(708,570,64,785,*)*/
static DEFINE_IDR(board_cdev_idr);
/*lint -e708 -e570 -e64 -e785 +esym(708,570,64,785,*)*/

/*lint -e651 -e708 -e570 -e64 -e785 -esym(651,708,570,64,785,*)*/
static DEFINE_MUTEX(board_cdev_lock);
/*lint -e651 -e708 -e570 -e64 -e785 +esym(651,708,570,64,785,*)*/

static LIST_HEAD(board_cdev_list);

static int get_idr(struct idr *idr, int *id)
{
	int ret;

	mutex_lock(&board_cdev_lock);
	ret = idr_alloc(idr, NULL, 0, 0, GFP_KERNEL);
	mutex_unlock(&board_cdev_lock);
	if (unlikely(ret < 0)) //lint !e730
		return ret;
	*id = ret;

	return 0;
}

static int release_idr(struct idr *idr, int id)
{
	mutex_lock(&board_cdev_lock);
	idr_remove(idr, id);
	mutex_unlock(&board_cdev_lock);

	return 0;
}


static int board_state2power(struct thermal_cooling_device *cdev,
			       struct thermal_zone_device *tz,
			       unsigned long state, u32 *power)
{
	struct board_cooling_device *board_cdev = cdev->devdata;

	if (tz == NULL)
		return -EINVAL;

	if (state > board_cdev->max_level)
		return -EINVAL;

	*power = board_cdev->power_table[state];

	return 0;
}

static int board_power2state(struct thermal_cooling_device *cdev,
			       struct thermal_zone_device *tz, u32 power,
			       unsigned long *state)
{
	struct board_cooling_device *board_cdev = cdev->devdata;
	u32 *power_table = board_cdev->power_table;
	unsigned long i;

	if (tz == NULL)
		return -EINVAL;

	for (i = 0; i < board_cdev->max_level; i++)
		if (power >= power_table[i])
			break;

	*state = i;

	return 0;
}

static int board_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct board_cooling_device *board_cdev = cdev->devdata;

	*state = board_cdev->max_level;

	return 0;
}

static int board_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct board_cooling_device *board_cdev = cdev->devdata;

	*state = board_cdev->current_level;

	return 0;
}

static int board_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	struct board_cooling_device *board_cdev = cdev->devdata;

	if (state == board_cdev->current_level)
		return 0;

	if (state > board_cdev->max_level)
		return -EINVAL;

	board_cdev->current_level = (unsigned int)state;

	return 0;
}

static struct thermal_cooling_device_ops board_cooling_ops = {
	.get_max_state = board_get_max_state,
	.get_cur_state = board_get_cur_state,
	.set_cur_state = board_set_cur_state,
	.state2power = board_state2power,
	.power2state = board_power2state,
};

static int board_get_requested_power(struct thermal_cooling_device *cdev,
				       struct thermal_zone_device *tz,
				       u32 *power)
{
	struct board_cooling_device *board_cdev = cdev->devdata;

	if (tz == NULL)
		return -EINVAL;

	*power = board_cdev->get_power();

	return 0;
}

struct thermal_cooling_device *
board_power_cooling_register (struct device_node *np, get_power_t get_power)
{
	int ret;
	struct board_cooling_device *board_cdev;
	struct thermal_cooling_device *cool_dev;
	char name[THERMAL_NAME_LENGTH];

	ret = of_property_count_elems_of_size(np, "power", (int)sizeof(u32));

	if (ret < 0) {
		pr_err("missing power property\n");
		return ERR_PTR((long)ret);
	}

	board_cdev = kzalloc(sizeof(*board_cdev), GFP_KERNEL);
	if (!board_cdev)
		return ERR_PTR((long)-ENOMEM);

	board_cdev->max_level = (unsigned int)ret;
	board_cdev->power_table = kzalloc(sizeof(*board_cdev->power_table) *
					  board_cdev->max_level, GFP_KERNEL);
	if (!board_cdev->power_table) {
		cool_dev = ERR_PTR((long)-ENOMEM);
		goto free_board_cdev;
	}

	ret = of_property_read_u32_array(np, "power", board_cdev->power_table, (size_t)board_cdev->max_level);
	if (ret) {
		pr_err("%s actor power read err\n", __func__);
		cool_dev = ERR_PTR((long)ret);
		goto free_power_table;
	}

	board_cdev->max_level--;

	board_cdev->get_power = get_power;
	board_cooling_ops.get_requested_power = board_get_requested_power;

	ret = get_idr(&board_cdev_idr, &board_cdev->id);
	if (ret) {
		cool_dev = ERR_PTR((long)ret);
		goto free_power_table;
	}

	snprintf(name, sizeof(name), "thermal-board-%d",
		 board_cdev->id);

	cool_dev = thermal_of_cooling_device_register(np, name, board_cdev,
						      &board_cooling_ops);
	if (IS_ERR(cool_dev))
		goto remove_idr;

	mutex_lock(&board_cdev_lock);
	list_add(&board_cdev->node, &board_cdev_list);
	mutex_unlock(&board_cdev_lock);

	return cool_dev; /*lint !e429*/

remove_idr:
	release_idr(&board_cdev_idr, board_cdev->id);
free_power_table:
	kfree(board_cdev->power_table);
free_board_cdev:
	kfree(board_cdev);

	return cool_dev;
}

EXPORT_SYMBOL(board_power_cooling_register);

void board_cooling_unregister(struct thermal_cooling_device *cdev)
{
	struct board_cooling_device *board_cdev;

	if (!cdev)
		return;

	board_cdev = cdev->devdata;

	mutex_lock(&board_cdev_lock);
	list_del(&board_cdev->node);
	mutex_unlock(&board_cdev_lock);

	thermal_cooling_device_unregister(board_cdev->cdev);
	release_idr(&board_cdev_idr, board_cdev->id);
	kfree(board_cdev->power_table);
	kfree(board_cdev);
}
EXPORT_SYMBOL(board_cooling_unregister);
