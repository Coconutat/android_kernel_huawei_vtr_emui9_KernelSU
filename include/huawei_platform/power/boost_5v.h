#ifndef _BOOST_5V_H_
#define _BOOST_5V_H_

#define BOOST_5V_ENABLE      (1)
#define BOOST_5V_DISABLE     (0)

typedef enum boost_ctrl_source_type {
	BOOST_CTRL_BEGIN = 0,

	BOOST_CTRL_WIRELESS_OTG = BOOST_CTRL_BEGIN,
	BOOST_CTRL_PD_VCONN,
	BOOST_CTRL_DC,
	BOOST_CTRL_MOTOER,
	BOOST_CTRL_AUDIO,
	BOOST_CTRL_AT_CMD,
	BOOST_CTRL_FCP,
	BOOST_CTRL_WLDC,

	BOOST_CTRL_MAX,
} boost_ctrl_source_type;

int boost_5v_enable(bool enable, boost_ctrl_source_type type);

#endif /* end of _BOOST_5V_H_ */