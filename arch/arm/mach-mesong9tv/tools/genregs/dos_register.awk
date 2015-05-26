#!/usr/bin/awk -f


BEGIN {
	print "/*"
	print " * "ARGV[1]
	print " */"
}


function printformat(REGNAME, REGADDR)
{
	REGLENG = length(REGNAME)

	if (REGLENG < 8)
		print "#define", REGNAME "\t\t\t\t\t"	REGADDR
	else if (REGLENG >=  8  && REGLENG < 16)
		print "#define", REGNAME "\t\t\t\t"	REGADDR
	else if (REGLENG >= 16  && REGLENG < 24)
		print "#define", REGNAME "\t\t\t"	REGADDR
	else if (REGLENG >= 24  && REGLENG < 32)
		print "#define", REGNAME "\t\t"		REGADDR
	else if (REGLENG >= 32  && REGLENG < 40)
		print "#define", REGNAME "\t"		REGADDR
}


#
# start to analysis the following syntax code.
# #define VDEC_ASSIST_MMC_CTRL0                      ((0x0001  << 2) + 0xd0050000)
# #define VDEC_ASSIST_MMC_CTRL1                      ((0x0002  << 2) + 0xd0050000)
#
/^#define/ && NF == 7 {

	REGNAME = $2

	match($0, /(0[xX][A-Fa-f0-9]+)/)
	REGADDR = substr($0, RSTART, RLENGTH)

	REGBASE = "DOS_REG_ADDR"


	printformat(REGNAME, REGADDR)
	printformat("P_"REGNAME, REGBASE "(" REGNAME ")")
}



END {
	print "\n"
}

