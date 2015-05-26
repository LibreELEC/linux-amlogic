genregs tool
=============================================

Use the following command to generate register.h automaticly:

./genregs.sh > register_out.h

Compare the generated file register_out.h with include/mach/register.h.
And update include/mach/register.h.



You can only generate one of the ucode header file.

For example, you can use the following command to generate DMC register
from the original ucode header file ucode/dmc_reg.h:

./dmc_reg.awk ucode/dmc_reg.h > dmc_reg_out.h


