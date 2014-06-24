/*
 * Copyright (c) 2014 EditorConfig Team
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*!
 * @file editorconfig/editorconfig_filesystem.h
 * @brief Header file of EditorConfig file system.
 *
 * @author EditorConfig Team
 */

#ifndef __EDITORCONFIG_EDITORCONFIG_FILESYSTEM_H__
#define __EDITORCONFIG_EDITORCONFIG_FILESYSTEM_H__

/* When included from a user program, EDITORCONFIG_EXPORT may not be defined,
 * and we define it here*/
#ifndef EDITORCONFIG_EXPORT
# define EDITORCONFIG_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* editorconfig_filesystem_pointer;
typedef struct editorconfig_filesystem editorconfig_filesystem;
typedef void* editorconfig_conf_file_iterator;

/*!
 * @brief The editorconfig filesystem object type
 */
struct editorconfig_filesystem {
    int (*check_filename)(editorconfig_filesystem*, const char*);
    char* (*normalize_filename)(editorconfig_filesystem*, const char*);
    void (*free_normalize_filename)(editorconfig_filesystem*, char*);
    editorconfig_conf_file_iterator (*list_conf_files)(editorconfig_filesystem*, const char*, const char*);
    const char* (*next_conf_file)(editorconfig_conf_file_iterator it);
    void (*free_list_conf_files)(editorconfig_conf_file_iterator);
    editorconfig_filesystem_pointer (*open_file)(editorconfig_filesystem*, const char*);
    char* (*read_file)(editorconfig_filesystem*, char *, int, editorconfig_filesystem_pointer);
    void (*close_file)(editorconfig_filesystem*, editorconfig_filesystem_pointer);
};

/*!
 * @brief Create and initialize a default editorconfig_filesystem object.
 *
 * @retval NULL Failed to create the editorconfig_filesystem object.
 *
 * @retval non-NULL The created editorconfig_filesystem object is returned.
 */
EDITORCONFIG_EXPORT
editorconfig_filesystem* editorconfig_filesystem_default_init(void);

/*!
 * @brief Destroy an editorconfig_filesystem object
 *
 * @param h The editorconfig_filesystem object needs to be destroyed.
 *
 * @retval zero The editorconfig_filesystem object is destroyed successfully.
 * 
 * @retval non-zero Failed to destroy the editorconfig_filesystem object.
 */
EDITORCONFIG_EXPORT
int editorconfig_filesystem_default_destroy(editorconfig_filesystem* fs);


EDITORCONFIG_EXPORT
editorconfig_filesystem* editorconfig_filesystem_reflect_init(const char* filename);

EDITORCONFIG_EXPORT
int editorconfig_filesystem_reflect_destroy(editorconfig_filesystem* fs);


#ifdef __cplusplus
}
#endif

#endif /* !__EDITORCONFIG_EDITORCONFIG_FILESYSTEM_H__ */

