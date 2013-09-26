#include <canopus/nvram.h>

// TODO rename me "nvram_cubebug2.c"

const nvram_t nvram_default = {
    .hdr = { .version = NVRAM_VERSION_CURRENT },
    .platform = {
    		.reset_count = 0,
    		.last_boot_reason = NULL,
    },
    .cdh = {
		.last_seen_sequence_number        = 0,
		.persist_sequence_number		  = false,
		.command_response_delay_ms        = 150, // delay before sending command response
		                                    /*1234567890123456789012345678901-*/
		.telecommand_key				  = {"You must change this for a key."},
		.FDIR_CDH_LAST_COMMAND_TIMEOUTs	  = 60*60*12,	// 12 hours with no incomming messages will reboot
        .FDIR_CDH_last_command_timeout_mask = FDIR_CDH_LAST_COMMAND_TIMEOUT_MASK_RESET_ALL,
        .one_time_radio_silence_time_s    = 20*60,

         /* Radio Configuration */
		.default_lithium_configuration    = LITHIUM_CONFIGURATION_DEFAULT_VALUES,
		.change_bps_on_every_boot		  = true,

        /* Beacon settings */

        /* Antenna Deployment settings */
        .antenna_deploy_delay_s			  	= 60*60,
        .antenna_deploy_enabled	          	= true,
        .antenna_deploy_tries_counter		= 0,

        /* extra functionality */
        .APRS_eanbled						= true,
    },
    .aocs = {
        .Bxw_gain = BXW_DEFAULT_NVRAM_GAIN,
        .detumbling = AOCS_DETUMBLING_BXW,
		.no_mtq_actuate = 0,
		.default_controller_frequency_ms = 1000,
		.run_sampler = AOCS_START_CDH_SAMPLER,
		.sampler_configuration = {
				.offset = 3+47+12,  /* header + aocs offset + gyro offset */
				.size = 6,   /* source_size  */
				.count = 270, /* cnt (3 orbits)    */
				.interval = 45,  /* interval (~1 min)   */
		},
		.default_mode = AOCS_DETUMBLING,
		.magcal_matrix_signed_raw = {{1.231079f,-0.051036f,0.021997f},
				                     {-0.051036f,1.159001f,0.023804f},
				                     {0.021997f,0.023804f,1.112006f}},
		.max_survival_dipole = .31, /* 25% taking into account dready mtq off */
		.mtq_axis_map = {
				.x_channel = 1, // PWM_1 is Y. + is -
				.x_sign = -1,
				.y_channel = 0, // PWM_2 is X. + is -
				.y_sign = -1,
				.z_channel = 2, // PWM_3 is Z. + is -
				.z_sign = -1,
		},
		.lovera_prefeed_conf = {
				.epsilon = .000005 * 4,
				.kp = 50,
				.kv = 50,
				.beta = .2,
		},
    },
    .mm = {
        .logmask                 = LOGMASK_DEFAULT,
        .inter_bulk_cmd_delay_ms = DEFAULT_INTER_BULK_CMD_DELAY_ms,
    },
    .power = {
    		.telemetry_update_time_s    = 10,
    		.battery_low_voltage_v      = 7.40f,
    		.battery_ok_voltage_v       = 7.55f,
    		.battery_mission_voltage_v  = 8.00f,
    	    .matrix_disable_key		    = MATRIX_KEY_ENABLED,
    		.imu_power_forced_off		= false,
    },
    .thermal = {
    		.thermal_limits_survival  = DEFAULT_THERMAL_LIMITS_SURVIVAL,
    		.thermal_limits_mission   = DEFAULT_THERMAL_LIMITS_MISSION,
    		.thermal_limits_low_power = DEFAULT_THERMAL_LIMITS_LOW_POWER,
    	    .matrix_disable_key       = MATRIX_KEY_DISABLED,
    },
    .payload = {
    		.experiments_enabled = true,
    		.total_run = 0,
    		.total_failed_count = 0,
    		.last_run_test = -1,
    },
};
