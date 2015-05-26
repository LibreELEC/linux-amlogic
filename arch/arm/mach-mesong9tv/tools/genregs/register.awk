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
# #define VDIN0_OFFSET            0x00
# #define VDIN1_OFFSET            0x70
#
/^#define([[:space:]]*)([A-Z0-9_]+)([[:space:]]*)(0[xX][A-Fa-f0-9]+)/{

	REGNAME = $2
	REGADDR = $3

	printformat(REGNAME, REGADDR)
}


#
# start to analysis the following syntax code.
# #define STB_TOP_CONFIG	((0x16f0  << 2) + 0xc1100000)
# #define VDIN_SCALE_COEF_IDX	((0x1200  << 2) + 0xd0100000)
#
/^#define/ && NF == 7 {

	REGNAME = $2

        if($2 ~/^SAR_ADC_/)
                next

	match($0, /(0[xX][A-Fa-f0-9]+)/)
	REGADDR = substr($0, RSTART, RLENGTH)

	if ($0 ~/0xc1100000/) {
		REGBASE = "CBUS_REG_ADDR"
	}
	else if ($0 ~/0xd0100000/) {
		REGBASE = "VCBUS_REG_ADDR"
	}
	else
		next

	printformat(REGNAME, REGADDR)
	printformat("P_"REGNAME, REGBASE "(" REGNAME ")")
}

#
# start to analysis the following syntax code.
# #define STB_TOP_CONFIG	((0x16f0  << 2) + 0xc1100000)
# #define VDIN_SCALE_COEF_IDX	((0x1200  << 2) + 0xd0100000)
#
/^#define/ && /HHI_HDMI_PLL_CNTL/ && NF == 7 {

	REGNAME = $2

	match($0, /(0[xX][A-Fa-f0-9]+)/)
	REGADDR = substr($0, RSTART, RLENGTH)

	if ($0 ~/0xc1100000/) {
		REGBASE = "CBUS_REG_ADDR"
	}
	else if ($0 ~/0xd0100000/) {
		REGBASE = "VCBUS_REG_ADDR"
	}
	else
		next

	sub(/HHI_HDMI_PLL_CNTL/, "HHI_VID_PLL_CNTL", REGNAME)

	printformat(REGNAME, REGADDR)
	printformat("P_"REGNAME, REGBASE "(" REGNAME ")")
}


#
# start to analysis the following syntax code.
# #define VDIN0_SCALE_COEF_IDX                    ((VDIN0_OFFSET << 2) + VDIN_SCALE_COEF_IDX               )
# #define VDIN1_SCALE_COEF_IDX                    ((VDIN1_OFFSET << 2) + VDIN_SCALE_COEF_IDX               )
#
/^#define/ && NF == 8 {

	REGNAME = $2
	REGADDR = $3" "$4" "$5" "$6" "$7$8

	PHYNAME = "P_"REGNAME
	PHYADDR = "VCBUS_REG_ADDR" "(" REGNAME ")"

	printformat(REGNAME, REGADDR)
	printformat(PHYNAME, PHYADDR)
}


END {
	print "\n"
}

