#ifndef TAROT_MANAGER_H
#define TAROT_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Source of the tarot view: mounts the SD card, lists the cards under
 * /sdcard/tarot/images and decodes a random one to the mono panel.
 */

/* Mount the SD card and list the card files. Safe before the display exists. */
void Tarot_ManagerInit(void);

/* Decode and display a random card, avoiding an immediate repeat. Falls back to
 * a status message when the card or its images are missing. */
void Tarot_ShowRandom(void);

#ifdef __cplusplus
}
#endif

#endif
