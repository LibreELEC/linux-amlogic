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
# #define DMC_REG_BASE      0xc8006000
#
/^#define([[:space:]]*)([[:alnum:]]+)/ && NF == 3 {

	REGNAME = "S_"$2
	REGADDR = "0x" substr($3, 7, 4);

	printformat(REGNAME, REGADDR)
}


#
# start to analysis the following syntax code.
# #define DMC_REQ_CTRL        DMC_REG_BASE + (0x00 << 2)
# #define DMC_SOFT_RST        DMC_REG_BASE + (0x01 << 2)
#
/^#define/ && /DMC_REG_BASE/ && NF == 7 {

	REGNAME = $2
	REGADDR = "(" "S_" $3 " " $4 " " $5 " " $6 " " $7 ")"

	PHYNAME = "P_"REGNAME
	PHYADDR = "MMC_REG_ADDR" "(" REGNAME ")"

	printformat(REGNAME, REGADDR)
	printformat(PHYNAME, PHYADDR)

}


END {
	print "\n"
}

