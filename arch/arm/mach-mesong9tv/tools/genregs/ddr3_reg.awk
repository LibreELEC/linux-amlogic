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
# #define AM_DDR_PLL_CNTL     0xc8000400
# #define AM_DDR_PLL_CNTL1    0xc8000404
#
/^#define([[:space:]]*)([A-Z0-9_]+)([[:space:]]*)(0[xX][A-Fa-f0-9]+)/  && NF == 3 {

	REGNAME = $2
	REGADDR = "0x" substr($3, 7, 10)

	if (REGNAME ~ /^P_/)
		REGNAME = substr(REGNAME, 3, length(REGNAME))

	PHYNAME = "P_" REGNAME
	PHYADDR = "MMC_REG_ADDR" "(" REGNAME ")"

	printformat(REGNAME, REGADDR)
	printformat(PHYNAME, PHYADDR)
}


#
# start to analysis the following syntax code.
#
# #define P_DDR1_PUB_RIDR               0xc8003000 + (0x00 << 2)
# #define P_DDR1_PUB_PIR                0xc8003000 + (0x01 << 2)
#
/^#define/ && NF == 7 {

	REGNAME = $2
	REGADDR = "0x" substr($3, 7, 10);
	REGADDR = "(" REGADDR " " $4 " " $5 " " $6 " " $7 ")"

	if (REGNAME ~ /^P_/)
		REGNAME = substr(REGNAME, 3, length(REGNAME))

	PHYNAME = "P_" REGNAME
	PHYADDR = "MMC_REG_ADDR" "(" REGNAME ")"

	printformat(REGNAME, REGADDR)
	printformat(PHYNAME, PHYADDR)
}



END {
	print "\n"
}

