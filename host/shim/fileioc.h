/* Host mirror of the CE fileioc subset used by save.c. AppVars are files in $SHIM_DIR (default build/appvars). */
#ifndef SHIM_FILEIOC_H
#define SHIM_FILEIOC_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

extern int shim_fail_archive;   /* nonzero: ti_SetArchiveStatus fails */

uint8_t ti_Open(const char *name, const char *mode);   /* handle or 0 */
int ti_Close(uint8_t handle);
size_t ti_Write(const void *data, size_t size, size_t count, uint8_t handle);   /* items written */
size_t ti_Read(void *data, size_t size, size_t count, uint8_t handle);          /* items read */
uint16_t ti_GetSize(uint8_t handle);
int ti_SetArchiveStatus(uint8_t archive, uint8_t handle);   /* 0 on failure */
#define ti_SetArchiveStatus(archive, handle) ti_SetArchiveStatus((bool)(archive), (handle))
int ti_Delete(const char *name);   /* 0 on failure */
uint8_t ti_Rename(const char *old_name, const char *new_name);   /* 0 ok, 1 exists, 2 other */
#endif
