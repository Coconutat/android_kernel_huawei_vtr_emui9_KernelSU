#define END_SYMBOL 0
struct pinctrl_sec_num {
	char *name;
	unsigned int gpio;
};

struct pinctrl_sec_num pin_num_name_table [] = {

	{"gpio144_cfg_func", 144},
	{"gpio145_cfg_func", 145},
	{"gpio146_cfg_func", 146},
	{"gpio147_cfg_func", 147},
	{"gpio207_cfg_func", 207},
	{"gpio208_cfg_func", 208},
	{"gpio209_cfg_func", 209},
	{"gpio210_cfg_func", 210},
	{"gpio211_cfg_func", 211},
	{"gpio212_cfg_func", 212},
	{"gpio213_cfg_func", 213},
	{"gpio214_cfg_func", 214},
	{"gpio215_cfg_func", 215},
	{"gpio216_cfg_func", 216},
	{"gpio217_cfg_func", 217},
	{"gpio218_cfg_func", 218},
	{"gpio219_cfg_func", 219},
	{"gpio220_cfg_func", 220},
	{"gpio221_cfg_func", 221},
	{"gpio008_cfg_func", 8},
	{"gpio009_cfg_func", 9},
	{"gpio010_cfg_func", 10},
	{"gpio011_cfg_func", 11},
	{"gpio012_cfg_func", 12},
	{"gpio013_cfg_func", 13},
	{"gpio014_cfg_func", 14},
	{"gpio015_cfg_func", 15},
	{"gpio016_cfg_func", 16},
	{"gpio017_cfg_func", 17},
	{"gpio018_cfg_func", 18},
	{"gpio019_cfg_func", 19},
	{"gpio020_cfg_func", 20},
	{"gpio021_cfg_func", 21},
	{"gpio022_cfg_func", 22},
	{"gpio023_cfg_func", 23},
	{"gpio024_cfg_func", 24},
	{"gpio025_cfg_func", 25},
	{"gpio026_cfg_func", 26},
	{"gpio029_cfg_func", 29},
	{"gpio030_cfg_func", 30},
	{"-",0},
};

struct pinctrl_sec_num *find_io_id(char *name)
{
	struct pinctrl_sec_num  *io_info_entry = NULL;
        int index = 0;

        io_info_entry = (struct pinctrl_sec_num *)(&pin_num_name_table[index]);

        while (END_SYMBOL !=  io_info_entry->gpio) {
		if (!strcmp(name, io_info_entry->name)) { /*lint !e421*/
                            return io_info_entry;
                    }
                    index++;
                    io_info_entry = (struct pinctrl_sec_num  *)(&pin_num_name_table[index]);
            }

            return NULL;
}

static int get_sec_pin_id(char *name)
{
	struct pinctrl_sec_num *io_info_entry = NULL;

	io_info_entry = find_io_id(name);
	if(io_info_entry == NULL) {
		return -ENOMEM;
	}
	return io_info_entry -> gpio;
}
