/*
 * delayed_cmds.h
 *
 *  Created on: Mar 6, 2013
 *      Author: gera
 */

#ifndef DELAYED_CMDS_H_
#define DELAYED_CMDS_H_

retval_t cmd_delayed_add(const subsystem_t *self, frame_t *iframe, frame_t *oframe, uint32_t sequence_number);
retval_t cmd_delayed_list(const subsystem_t *ss, frame_t *iframe, frame_t *oframe);
retval_t cmd_delayed_discard(const subsystem_t *ss, frame_t *iframe, frame_t *oframe);

void cmd_delayed_init(void);

#endif /* DELAYED_CMDS_H_ */
