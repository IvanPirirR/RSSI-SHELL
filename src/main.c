/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <stdio.h>
#include <device.h>
#include <devicetree.h>
#include <shell/shell.h>
#include <logging/log.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/scan.h>
#include <sys/byteorder.h>

static void config_scan(void);
static struct bt_scan_cb scan_cb;
static const struct shell *main_shell;

static int active_print;
static int rssiVal;
static int average_rssi[50] ={0};
static int rssi_number = 0;


/* Scanning for Advertising packets, using the name to check if the device is the target*/
static void config_scan(void)
{
	int err;
	char *target = "Default";

	//General scan parameters
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = 0x0010,
		.window = 0x0010,
	};

	//Parameters for matching
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 0,
		.scan_param = &scan_param,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT,
	};
	
	//Initializing scan and registering callback functions
	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);
	
	// Add the target name to the filter
	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, target);
	if (err) {
		shell_print(main_shell,"Scanning filters cannot be set\n");
		return;
	}

	//Enable the filter. The flag is set to false as there is only one filter.
	err = bt_scan_filter_enable(BT_SCAN_NAME_FILTER, false);
	if (err) {
		shell_print(main_shell,"Filters cannot be turned on\n");
	}
}

//Callback function for a matching name
void scan_filter_match(struct bt_scan_device_info *device_info,
		       struct bt_scan_filter_match *filter_match,
		       bool connectable)
{

	//Get the rssi
	rssiVal=device_info->recv_info->rssi;

	if (active_print == 1){
		shell_print(main_shell,"rssi: %i",rssiVal);
	}

	if (rssi_number == 20){
		rssi_number = 0;
	}
	average_rssi[rssi_number] = rssiVal;
	rssi_number++;
}


//Scanning error
void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	shell_print(main_shell,"Connection to peer failed!\n");
}


//Declaring callback functions for the scan filter
BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, NULL);



//Shell
static int cmd_print_start(const struct shell *shell, size_t argc,
				    char **argv, void *data)
{
	active_print = 1;
	main_shell = shell;
	shell_print(shell,"Active rssi print on");
	return 0;
}

static int cmd_print_stop(const struct shell *shell, size_t argc,
				    char **argv, void *data)
{
	active_print =0;
	shell_print(shell,"Active rssi print off");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_print_rssi,
		SHELL_CMD_ARG(start,NULL,
				"Start printing rssi value in real time",
				cmd_print_start,1,0),
		SHELL_CMD_ARG(stop,NULL,
				"Stop active print",
				cmd_print_stop,1,0),
				SHELL_SUBCMD_SET_END
);


static int cmd_rssi_signal(const struct shell *shell, size_t argc,
				    char **argv)
{
	shell_print(shell,"Latest rssi: %i",rssiVal);
	return 0;
}

static int cmd_average_rssi(const struct shell *shell, size_t argc,
				    char **argv)
{
	int tot = 0;
	int values = 50; 
	for (int i =0; i<50 ;i++)
	{
		if (average_rssi[i] == 0){
			values = i+1;
			break;
		}
		tot += average_rssi[i];
	}
	tot = tot/values;
	
	shell_print(shell,"Average rssi value %i",tot);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_rssi,
		SHELL_CMD_ARG(read, NULL,
				"Displays the current rssi signal",
				cmd_rssi_signal,1,0),
		SHELL_CMD_ARG(average, NULL,
				"Displays the average rssi value",
				cmd_average_rssi,1,0),
		SHELL_CMD_ARG(print, &sub_print_rssi,
				"Prints the rssi value in real time",
				NULL,2,0),
				SHELL_SUBCMD_SET_END 
);

static int cmd_passive_start(const struct shell *shell, size_t argc,
				    char **argv, void *data)
{
	int err;
	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
			if (err == -EALREADY)
			{
				shell_print(shell,"Scanning already enable \n");
			}
			else if (err) {
				shell_print(shell,"Scanning failed to start, err %d\n", err);
			}
			else{
				shell_print(shell,"Passive scanning on");
			}
	return 0;
}

static int cmd_active_start(const struct shell *shell, size_t argc,
				    char **argv, void *data)
{
	int err;
	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
			if (err == -EALREADY)
			{
				shell_print(shell,"Scanning already enable \n");
			}
			else if (err) {
				shell_print(shell,"Scanning failed to start, err %d\n", err);
			}
			else{
				shell_print(shell,"Active scanning on");
			}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_start_scan,
		SHELL_CMD_ARG(passive, NULL,
				"Start passive scanning",
				cmd_passive_start, 1,0),
		SHELL_CMD_ARG(active, NULL,
				"Start active scanning",
				cmd_active_start, 1,0),
				SHELL_SUBCMD_SET_END
);

static int cmd_enable_scan(const struct shell *shell, size_t argc,
				    char **argv)
{
	int err;
	main_shell = shell;
	err = bt_enable(NULL);
		if (err) {
			shell_print(shell,"Cold not enable Bluetooth\n");
		}
		else{
			shell_print(shell,"Bluetooth initialized\n");
			config_scan();
			shell_print(shell,"Scanning module enable \n");
		}

	return 0;
}

static int cmd_stop_scan(const struct shell *shell, size_t argc,
				    char **argv)
{
	int err;
	err = bt_scan_stop();
	if (err == -EALREADY) {
		shell_print(shell,"Scanning is not on \n");
	}
	else if (err){
		shell_print(shell,"Scanning failed to stop, err %d\n", err);
	}
	else {
		shell_print(shell,"Scanning has stopped \n");
	}
	return 0;
}

static int cmd_change_name(const struct shell *shell, size_t argc,
				    char **argv)
{
	bt_scan_filter_remove_all();
	int err;

	// Add the target name to the filter
	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, argv[1]);
	if (err) {
		shell_print(shell,"Scanning filters cannot be set\n");
	} 
	else{
		shell_print(shell,"The target device name was set to: %s",argv[1]);
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bt_scan,
		SHELL_CMD_ARG(enable, NULL,
				"Set default configuration and enable scanning",
				cmd_enable_scan,1,0),
		SHELL_CMD_ARG(start, &sub_start_scan,
				"Start scanning",
				NULL,2,0),
		SHELL_CMD_ARG(stop,NULL,
				"Stop scanning",
				cmd_stop_scan,1,0),
		SHELL_CMD_ARG(rssi, &sub_rssi,
				"Display options for the Received signal strength indicator (rssi)",
				NULL,2,1),
		SHELL_CMD_ARG(change_name, NULL,
				"Change the name of the target device <value: name>",
				cmd_change_name,2,0),
				SHELL_SUBCMD_SET_END
);


SHELL_CMD_REGISTER(bt_scan, &bt_scan, "Enable options for passive and active scanning", NULL);

//Main
void main(void)
{
	#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
	const struct device *dev;
	uint32_t dtr = 0;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	if (!device_is_ready(dev) || usb_enable(NULL)) {
		return;
	}

	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}
	#endif

}
