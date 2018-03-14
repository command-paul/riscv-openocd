//------------------------- [IITM] Shakti LockStep Verification framework 
//------------------------- Command Extentions to the RISCV openocd project
//------------------------- Paul George - Shiv Nadar University

/***************************************************************************
 *   Copyright (C) 2017 Paul George			                               *
 *   pg456@snu.edu.in 	                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/


// Template Guide to writing your own commands - Luap Egoreg
	// {
	// 	.name = "0", 									// Any of the 94 printable ASCII characters for BW efficiency
	// 	.jim_handler = handleA,							// use jim handler if your implementing a command that needs to return some data 
	// 	.mode = COMMAND_EXEC,							// read ocd docs
	// 	.usage = "slsv A <hex>",						// a print message that goes out in response to the help command
	// 	.help = "this echos the <hex> 3 times",			// sort of like the aboveish 
	// },
	// 	{
	// 	.name = "1",
	// 	.handler = dump_csr,							// basic command handler type simple cases cant do formatted return without breaking OOCD (?AFAICS)
	// 	.mode = COMMAND_EXEC,
	// 	.help = "dump all 4096 csr`s",
	// 	.usage = "slsv dump csr",
	// },
	// {												// Bw efficiency is asymptotically 50% on data tf thanks to operating over telnet more overhead => lower throughput :(
	// 	.name = "dump",//.handler = halt,
	// 	.mode = COMMAND_EXEC,
	// 	.help = "slsv dump state variables command group",
	// 	.usage = "",
	// 	.chain = dump_state_command_handlers,			// When you NEED one more level prefix switching // define another {static const struct command_registration} and pass as argument
	// },





#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <helper/log.h>
#include "slsv.h"
#include "target/target.h"
#include "target/target_type.h"
#include "riscv.h"
#include <jim.h>


// static decls - is the static necessary ? - does it help - kindof helps reduce the slsv symbol clutter into oocd
// These will in future be updated to just be wrapper functions calling slsv version specific 

static int cmd_0_halt	(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_1_resume	(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_2_stepi	(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_3_stepn	(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_4_getAreg(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_5_setAreg(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_6_getMem	(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_7_setMem	(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_8_getDmi	(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_9_setDmi	(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_A_pollDmStatus(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_B_setSLSVversion(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_C_reset(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_D_addBreakpoint(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_E_removeBreakpoint(Jim_Interp *interp, int argc, Jim_Obj * const *argv);
static int cmd_F_shutdown(Jim_Interp *interp, int argc, Jim_Obj * const *argv);


// utility methods
static int getPackedHex(uint64_t* vector,uint count,char* hexString,uint width);

const struct command_registration slsv_exec_command_handlers[] = {
	{
		.name = "0",
		.jim_handler =cmd_0_halt,
		.mode = COMMAND_EXEC,
		.usage = "slsv A <hex>",
		.help = "this echos the <hex> 3 times",
	},
		{
		.name = "1",
		.jim_handler =cmd_1_resume,
		.mode = COMMAND_EXEC,
		.help = "dump all 4096 csr`s",
		.usage = "slsv dump csr",
	},
	{
		.name = "2",
		.jim_handler =cmd_2_stepi,
		.mode = COMMAND_EXEC,
		.help = "dump all 32 gpr`s",
		.usage = "slsv dump gpr",
	},
	{
		.name = "3",
		.jim_handler =cmd_3_stepn,
		.mode = COMMAND_EXEC,
		.help = "dump all 32 fpr`s",
		.usage = "slsv dump fpr",
	},
	{
		.name = "4",
		.jim_handler =cmd_4_getAreg,
		.mode = COMMAND_EXEC,
		.help = "current pc value",
		.usage = "slsv dump pc",
	},
	{
		.name = "5",
		.jim_handler =cmd_5_setAreg,
		.mode = COMMAND_EXEC,
		.help = "current dmi registers",
		.usage = "slsv dump dmi",
	},
	{
		.name = "6",
		.jim_handler =cmd_6_getMem,
		.mode = COMMAND_EXEC,
		.help = "current dmi register",
		.usage = "slsv dump DMI_reg <regno> regno",
	},
	{
		.name = "7",
		.jim_handler =cmd_7_setMem,
		.mode = COMMAND_EXEC,
		.usage = "slsv A <hex>",
		.help = "this echos the <hex> 3 times",
	},
		{
		.name = "8",
		.jim_handler =cmd_8_getDmi,
		.mode = COMMAND_EXEC,
		.help = "dump all 4096 csr`s",
		.usage = "slsv dump csr",
	},
	{
		.name = "9",
		.jim_handler =cmd_9_setDmi,
		.mode = COMMAND_EXEC,
		.help = "dump all 32 gpr`s",
		.usage = "slsv dump gpr",
	},
	{
		.name = "A",
		.jim_handler =cmd_A_pollDmStatus,
		.mode = COMMAND_EXEC,
		.help = "dump all 32 fpr`s",
		.usage = "slsv dump fpr",
	},
	{
		.name = "B",
		.jim_handler =cmd_B_setSLSVversion,
		.mode = COMMAND_EXEC,
		.help = "current pc value",
		.usage = "slsv dump pc",
	},
	{
		.name = "C",
		.jim_handler =cmd_C_reset,
		.mode = COMMAND_EXEC,
		.help = "current dmi registers",
		.usage = "slsv dump dmi",
	},
	{
		.name = "D",
		.jim_handler =cmd_D_addBreakpoint,
		.mode = COMMAND_EXEC,
		.help = "current dmi register",
		.usage = "slsv dump DMI_reg <regno> regno",
	},
	{
		.name = "E",
		.jim_handler =cmd_E_removeBreakpoint,
		.mode = COMMAND_EXEC,
		.help = "current dmi register",
		.usage = "slsv dump DMI_reg <regno> regno",
	},
	{
		.name = "F",
		.jim_handler =cmd_F_shutdown,
		.mode = COMMAND_EXEC,
		.help = "current dmi register",
		.usage = "slsv dump DMI_reg <regno> regno",
	},
	COMMAND_REGISTRATION_DONE
};


static int getPackedHex(uint64_t* vector,uint count,char* hexString,uint width){
	// witdth is the width in bytes :: 8 for 32b , 16 for 64b
	//*count = strlen(sexString)/width; // the maximum number of segments of width extractable
	//vector = malloc(sizeof(uint64_t)*count);  // This seems better off is done by the calling fucntion
	// the range 32-126 is 94 printable ascii characters , 32 being the space symbol where string parseing will break the string
	//LOG_ERROR("SLSV string:: %s, width :: %u" ,hexString,width);
	char temp[32+1]; // assuming the widest word rv128 is 128 bits wide // Plus one to hold the null terminator
	for(uint i = 0;i<count;i++){
		// put error catching on the returns of strncpy and sscanf
		strncpy(temp,(hexString+(i*width)),width);
		temp[width] = '\0';
		sscanf(temp,"%lx",vector+i);
		//LOG_ERROR("SLSV parsed integer:: %lu , string:: %s,string:: %s" ,*(vector+i),temp,(hexString+(count*width)));
	}
	return 0;
}

// Slsv Handler definitions
static int cmd_0_halt	(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	struct command_context *cmd_ctx = current_command_context(interp);
	struct target *target = get_current_target(cmd_ctx);
	retval= riscv_halt_all_harts(target);
	if (retval != ERROR_OK)	LOG_ERROR("Unable to halt all harts");
	LOG_INFO("Halted all HART`s");
	return retval;
}
static int cmd_1_resume	(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	struct command_context *cmd_ctx = current_command_context(interp);
	struct target *target = get_current_target(cmd_ctx);
	retval= riscv_resume_all_harts(target);
	if (retval != ERROR_OK) LOG_ERROR("Unable to resume all harts");
	// Return response
	LOG_INFO("Resumed all HART`s");
	return retval;
}
static int cmd_2_stepi	(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	struct command_context *cmd_ctx = current_command_context(interp);
	struct target *target = get_current_target(cmd_ctx);
	LOG_INFO("%s Steped\n",target->type->name)	; // some issue with the step function because the line si not printing any more :(
	target_addr_t addr = 0; // again carried forward from target.c
	int current_pc = 1; // This is just something carried forward from target.c
	target->type->step(target, current_pc, addr, 1);
	return retval;
}
static int cmd_3_stepn	(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	uint64_t steps;
	int len;
	char* s = (char*)Jim_GetString(argv[1],&len) ; //  Potential source of leaking memory  ?? get into spike docs to confirm 
	getPackedHex(&steps,1,s,16);
	steps = steps%65536;
	struct command_context *cmd_ctx = current_command_context(interp);
	struct target *target = get_current_target(cmd_ctx);
	target_addr_t addr = 0; // again carried forward from target.c
	int current_pc = 1; // This is just something carried forward from target.c
	LOG_INFO("%s Stepping %lu\n",target->type->name , steps)	;
	while(steps > 0){
		target->type->step(target, current_pc, addr, 1); //  alternatively use the target_step functions
		steps--;
		if (steps%1000 == 0)LOG_INFO("->step<-\n");
	}
	// pls put something to habdle errors
	return retval;
}
static int cmd_4_getAreg(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	int len;
	char* s = (char*)Jim_GetString(argv[1],&len) ; // look at openocd documentationa dn see if theis needs to be freed after use
	uint64_t reg_list[1024]; // this is hardcoded because i dont want to start a malloc free bug hunt just yet - max queriable at once is 1024
	getPackedHex(reg_list,len/5,s,5); //  the addresses are of len 5 because , although the spec needs only 4 , I`m adding one extra for more state vars
	// Parse Input
		// SLSV 4 <REGID><REGID><REGID>
		// Here Each REG ID is ABITS WIDE A special Sequence of ALL ABITS set to 1 is a range fetch specifier
	char response[8192];
	char* location=response;
	struct command_context *cmd_ctx = current_command_context(interp);
	struct target *target = get_current_target(cmd_ctx);
	for( int i = 0 ; i < len/5 ; i++ ) { 
		riscv_reg_t value;
		riscv_get_register_on_hart(target,&value,0,reg_list[i]); //there is an assert hartid ==0 in rv0.11c
		sprintf(location,"%016lx",value); // put some switch case on width ?? 
		//LOG_ERROR("reg %lx :: value %lx \n",reg_list[i],value);
		location+=16;
	}
	Jim_SetResultString(interp,response, -1);
	return retval;
}
static int cmd_5_setAreg(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	// Parse Input
		//SLSV 5 <abits><value>....<abits><value>
	// Process
	// Return response
	return retval;
}
static int cmd_6_getMem	(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	// Parse Input
		// SLSV 4 <read_width><address><address><address>
		// Here Each addres is width wide and ranges will be seperated by hyphens
	// Process
	// Return response
	return retval;
}
static int cmd_7_setMem	(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	// Parse Input
		// SLSV 7 width <address><mem><address><mem><address><mem><address><mem><address><mem>
	// Process
	// Return response
	return retval;
}
static int cmd_8_getDmi	(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	// Parse Input
	// Process
	// Return response
	return retval;
}
static int cmd_9_setDmi	(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	// Parse Input
	// Process
	// Return response
	return retval;
}
static int cmd_A_pollDmStatus(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	// Parse Input
	// Process
	// Return response
	return retval;
}
static int cmd_B_setSLSVversion(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	// Parse Input
	// Process
	// Return response
	return retval;
}
static int cmd_C_reset(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	// Parse Input
	// Process
	// Return response
	return retval;
}
static int cmd_D_addBreakpoint(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	// Parse Input
	// Process
	// Return response
	return retval;
}
static int cmd_E_removeBreakpoint(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	// Parse Input
	// Process
	// Return response
	return retval;
}
static int cmd_F_shutdown(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	int retval = JIM_OK;
	// Parse Input
	// Process
	// Return response
	return retval;
}




// Template JimCommand Handler
/*
static int handleA(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
	// one or two arguments, access a single register (write if second argument is given) 
	int retval =  JIM_OK;
	// if (argc < 1 || argc > 1) {
	// 	Jim_WrongNumArgs(interp, 1, argv, "addr [value]");
	// 	return JIM_ERR;
	// }

	struct command_context *cmd_ctx = current_command_context(interp);
	assert(cmd_ctx != NULL);

	struct target *target = get_current_target(cmd_ctx);
	if (target == NULL) {
		LOG_ERROR("arm946e: no current target");
		return JIM_ERR;
	}

	// if (target->state != TARGET_HALTED) {
	// 	command_print(cmd_ctx, "target %s must be stopped for \"cp15\" command", target_name(target));
	// 	return JIM_ERR;
	// }

	long HexBaba;
	int len;
	//const char* hex = Jim_GetString(argv[1], &len);
	sscanf(Jim_GetString(argv[1], &len),"%lx",&HexBaba);
	//retval = Jim_GetLong(interp, argv[1], &HexBaba);
	if (JIM_OK != retval)
		return retval;
	char buf[64];
	sprintf(buf, "0x%lx,0x%lx,0x%lx",(unsigned long)HexBaba,(unsigned long)HexBaba,(unsigned long)HexBaba);
	// Return value in hex format 
	Jim_SetResultString(interp, buf, -1);
	return JIM_OK;
}
*/
// End Template Jim handler

// static int cmd_A_step(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
// 	int retval =  JIM_OK;
// 	// if (argc < 1 || argc > 1) {
// 	// 	Jim_WrongNumArgs(interp, 1, argv, "addr [value]");
// 	// 	return JIM_ERR;
// 	// }

// 	struct command_context *cmd_ctx = current_command_context(interp);
// 	assert(cmd_ctx != NULL);

// 	struct target *target = get_current_target(cmd_ctx);
// 	if (target == NULL) {
// 		LOG_ERROR("arm946e: no current target");
// 		return JIM_ERR;
// 	}

// 	// if (target->state != TARGET_HALTED) {
// 	// 	command_print(cmd_ctx, "target %s must be stopped for \"cp15\" command", target_name(target));
// 	// 	return JIM_ERR;
// 	// }

// 	long HexBaba;
// 	int len;
// 	//const char* hex = Jim_GetString(argv[1], &len);
// 	sscanf(Jim_GetString(argv[1], &len),"%lx",&HexBaba);
// 	//retval = Jim_GetLong(interp, argv[1], &HexBaba);
// 	if (JIM_OK != retval)
// 		return retval;
// 	char buf[64];
// 	sprintf(buf, "0x%lx,0x%lx,0x%lx",(unsigned long)HexBaba,(unsigned long)HexBaba,(unsigned long)HexBaba);
// 	// Return value in hex format 
// 	Jim_SetResultString(interp, buf, -1);
// 	return JIM_OK;
// }



// COMMAND_HANDLER(resume){
// 	struct target *target = get_current_target(CMD_CTX);
// 	LOG_ERROR("Resumed\n");
// 	int current = 1;
// 	target_addr_t addr = 0;

// 	target->type->resume(target, current, addr,1, 0);
// 	return ERROR_OK;
// 	}


// COMMAND_HANDLER(step){
// 	struct target *target = get_current_target(CMD_CTX);
// 	LOG_ERROR("%s Steped\n",target->type->name)	;
// 	target_addr_t addr = 0;
// 	int current_pc = 1;
// 	target->type->step(target, current_pc, addr, 1);

// 	//target->type->step();
// 	return ERROR_OK;
// 	}


// COMMAND_HANDLER(halt){
// 	struct target *target = get_current_target(CMD_CTX);
// 	LOG_ERROR("%s Halted\n",target->type->name)	;
// 	target->type->halt(target);
// 	//target->type->step();
// 	return ERROR_OK;
// 	}


// COMMAND_HANDLER(dump_pc){
// 	struct target *target = get_current_target(CMD_CTX);
// 	LOG_ERROR("%s DEBUG\n",target->type->name)	;
// 	//Jim_Interp *interp = CMD_CTX->interp;
// 	//JimSetStringBytes(interp, "Hello, World!")
// 	//Jim_SetResultString(interp, "Hello, World!", -1);
// 	//LOG_ERROR("%s DEBUG\n",target->type->name)	;
// 	//LOG_ERROR("%s dumping CSR`s\n",target->type->name)	;
// 	//LOG_ERROR(" PC = %lx \n",riscv_get_register(target,32))	;
// 	//LOG_ERROR(" DMI 0x16 = %lx \n",dmi_read(target, 0x16))	;
// 	return ERROR_OK;
// 	}







	// {
	// 	.name = "step"
	// 	,
	// 	.handler = step,
	// 	.mode = COMMAND_EXEC,
	// 	.help = "used to print lol siddhart mitra , accepts a name"
	// 		"but still prints sidhart mitra",
	// 	.usage = "step",
	// },
	// {
	// 	.name = "resume",
	// 	.handler = resume,
	// 	.mode = COMMAND_EXEC,
	// 	.help = "display trace points, clear list of trace points, "
	// 		"or add new tracepoint at address",
	// 	.usage = "resume",
	// },
	// {
	// 	.name = "halt",
	// 	.handler = halt,
	// 	.mode = COMMAND_EXEC,
	// 	.help = "display trace points, clear list of trace points, "
	// 		"or add new tracepoint at address",
	// 	.usage = "halt",
	// },
	// {
	// 	.name = "setcsr",
	// 	.handler = set_csr,
	// 	.mode = COMMAND_EXEC,
	// 	.help = "set a csr regno value",
	// 	.usage = "slsv setcsr <regno> <value>",
	// },




// static int handleA(Jim_Interp *interp, int argc, Jim_Obj * const *argv){
// 	int retval =  JIM_OK;
// 	struct command_context *cmd_ctx = current_command_context(interp);
// 	assert(cmd_ctx != NULL);

// 	struct target *target = get_current_target(cmd_ctx);
// 	if (target == NULL) {
// 		LOG_ERROR("SLSV :: No target Detected");
// 		return JIM_ERR;
// 	}
// 	uint64_t* HexBaba;
// 	unsigned int len = 0;
// 	char* hex = (char*)Jim_GetString(argv[1],(int*) &len);
// 	HexBaba = malloc(sizeof(uint64_t)*len);
// 	uint wsym = (unsigned) (*hex);
// 	uint width = wsym -(unsigned) 32;
// 	//LOG_ERROR("SLSV wsym :: %c,%d,len :: %u ,width :: %u ,string :: %s" ,*hex,wsym, len,width,hex);
// 	getPackedHex(HexBaba,(len/width),(hex+1),width);
// 	//sscanf(Jim_GetString(argv[1], &len),"%lx",&HexBaba);
// 	char buf[64];// keeping this short for this does not seem to need to be short though
// 	char* location =buf; // like the linker location pointer
// 	uint64_t sum =0;
// 	for(uint i = 0;i < (len/width) ; i ++ ){
// 		sum+= HexBaba[i];
// 	}
// 	sprintf(location,"%016lx",sum); // put some switch case on width ?? 
// 	location+=width;
// 	sprintf(location,"%016lx",HexBaba[0]); // put some switch case on width ?? 
// 	location+=width;
// 	sprintf(location,"%016lx",HexBaba[1]); // put some switch case on width ?? 
// 	/* Return value in hex format */
// 	Jim_SetResultString(interp, buf, -1);
// 	free(HexBaba);
// 	return retval;
// }