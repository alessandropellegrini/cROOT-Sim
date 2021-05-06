# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

# Model helper library sources
rootsim_mods_srcs += files(
  'abm' / 'abm.c',
  'abm' / 'agent_map.c',
  'random' / 'random.c',
  'state' / 'state.c',
  'topology' / 'topology.c',
  'lib.c'
)

lib_objs = clang_with_plugin.process(rootsim_mods_srcs)

rootsim_mods_lib = static_library('rootsim-mods', lib_objs, install : true)
