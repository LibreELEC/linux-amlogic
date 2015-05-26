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
# #define STB_VERSION                                0x1600
# #define STB_VERSION_2                              0x1650
#
/^#define([[:space:]]*)([A-Z0-9_]+)([[:space:]]*)(0[xX][A-Fa-f0-9]+)/ {

	REGNAME = $2
	REGADDR = $3

	PHYNAME = "P_"REGNAME
	PHYADDR = "CBUS_REG_ADDR" "(" REGNAME ")"

	printformat(REGNAME, REGADDR)
	printformat(PHYNAME, PHYADDR)
}


END {
	print "\n"
}

