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
# #define P_AO_RTI_STATUS_REG0        (volatile unsigned long *)(0xc8100000 | (0x00 << 10) | (0x00 << 2))
# #define P_AO_RTI_STATUS_REG1        (volatile unsigned long *)(0xc8100000 | (0x00 << 10) | (0x01 << 2))
#
/^#define([[:space:]]*)P_AO_./ && NF == 14 {

	# AO_RTC_ADDR are defined in secure_apb.h on Secure APB3 bus
	if ($0 ~ /AO_RTC_ADDR/)
		next

	REGNAME = substr($2, 3, (length($2) - 2))
	REGADDR = "(" $8 " " $9 " " $10 " " $11 " " $12 " " $13 " " $14

	PHYNAME = "P_"REGNAME
	PHYADDR = "AOBUS_REG_ADDR" "(" REGNAME ")"

	printformat(REGNAME, REGADDR)
	printformat(PHYNAME, PHYADDR)
}


END {
	print "\n"
}

