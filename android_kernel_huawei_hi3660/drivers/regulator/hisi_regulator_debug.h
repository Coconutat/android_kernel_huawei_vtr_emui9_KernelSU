

#include <linux/regulator/driver.h>

#ifdef CONFIG_HISI_REGULATOR_TRACE

typedef enum {
	TRACK_ON_OFF,
	TRACK_VOL,
	TRACK_MODE,
	TRACK_REGULATOR_MAX
} track_regulator_type;

void track_regulator_onoff(struct regulator_dev *rdev, track_regulator_type track_item);
void track_regulator_set_vol(struct regulator_dev *rdev, track_regulator_type track_item, int max_uV, int min_uV);
void track_regulator_set_mode(struct regulator_dev *rdev, track_regulator_type track_item, u8 mode);
#endif
