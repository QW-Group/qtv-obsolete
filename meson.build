project('qtv', 'c')

if meson.get_compiler('c').get_id() == 'gcc'
    add_global_link_arguments('-static-libgcc', language: 'c')
endif

qtv_sources = [
	'ban.c',
	'build.c',
	'cl_cmds.c',
	'cmd.c',
	'crc.c',
	'cvar.c',
	'forward.c',
	'forward_pending.c',
	'fs.c',
	'httpsv.c',
	'httpsv_generate.c',
	'info.c',
	'main.c',
	'mdfour.c',
	'msg.c',
	'net_utils.c',
	'parse.c',
	'qw.c',
	'source.c',
	'source_cmds.c',
	'sys.c',
	'token.c',
	'sha3.c',
	'udp.c'
]

executable('qtv', qtv_sources,
	dependencies : [
		meson.get_compiler('c').find_library('ws2_32', required : false)
	]
)
