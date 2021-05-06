# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

# Model helper library sources
if not build_with_plugins
  rootsim_mods_srcs += files('lib.c')
endif

lib_objs = clang_with_plugin.process(rootsim_mods_srcs)

rootsim_mods_lib = static_library('rootsim-mods', lib_objs, install : true)
