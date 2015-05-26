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

/^\/\// && /Slot([[:space:]]*)0/ {
	REGBASE = "SECBUS_REG_ADDR"
}

/^\/\// && /Slot([[:space:]]*)2/ {
	REGBASE = "SECBUS2_REG_ADDR"
}

/^\/\// && /Slot([[:space:]]*)3/ {
	REGBASE = "SECBUS3_REG_ADDR"
}


#
# start to analysis the following syntax code.
# #define VDEC_ASSIST_MMC_CTRL0                      0x0001
# #define VDEC_ASSIST_MMC_CTRL1                      0x0002
#
/^#define([[:space:]]*)([A-Z0-9_]+)([[:space:]]*)(0[xX][A-Fa-f0-9]+)/{

	REGNAME = $2
	REGADDR = $3

	PHYNAME = "P_"REGNAME
	PHYADDR = REGBASE "(" REGNAME ")"

	printformat(REGNAME, REGADDR)
	printformat(PHYNAME, PHYADDR)
}


END {
	print "\n"
}

