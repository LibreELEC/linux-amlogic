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
#
# #define HDMITX_ADDR_PORT        0xd0042000  // TOP ADDR_PORT: 0xd0042000; DWC ADDR_PORT: 0xd0042010
# #define HDMITX_DATA_PORT        0xd0042004  // TOP DATA_PORT: 0xd0042004; DWC DATA_PORT: 0xd0042014
# #define HDMITX_CTRL_PORT        0xd0042008  // TOP CTRL_PORT: 0xd0042008; DWC CTRL_PORT: 0xd0042018
#
/^#define([[:space:]]*)HDMITX_([A-Z0-9_]+)([[:space:]]*)(0[xX][A-Fa-f0-9]+)/ {

	REGNAME = $2
	REGADDR = "0x" substr($3, 6, 10)

	PHYNAME = "P_" REGNAME
	PHYADDR = "APB_REG_ADDR" "(" REGNAME ")"

	printformat(REGNAME, REGADDR)
	printformat(PHYNAME, PHYADDR)
}



END {
	print "\n"
}

