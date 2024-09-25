//Pipelined processor simulation using C++. The following is capable of executing a few MIPS instruction and hence can execute any MIPS program using those MIPS instructions.

#include<iostream>
#include<fstream>
#include<unordered_map>
#include<vector>
using namespace std;

#define I_STARTING_ADDRESS 4194304               //Macro for defining the starting address of Instruction Memory.
#define D_STARTING_ADDRESS 268500992             //Macro for defining the starting address of Data Memory.
#define REG_NUMBERS 32                           //Macro for defining the total number of registers.
#define MEMORY_SPACE 100                         //Macro for defining the memory size.
#define NUM_OF_TRIES 3                           //Macro for defining how many times user can choose to execute the processor for programs.

long long int clock_cycle=1;                     //Counter for keeping track of the current cycle number.
long long int PC=I_STARTING_ADDRESS;             //Program Counter, loaded with the address of the first instruction to be executed.

vector<int> registers(32,0);                     //Registers.
vector<string> i_mem;                            //Instruction Memory.
vector<int> d_mem(MEMORY_SPACE,0);               //Data Memory.


//A map mapping the opcode of an instruction to the values of it's control signals.
//The opcodes are in order of the instructions  R-type,lw,sw,beq,j,addi,bne,jal.
//The control signals are in order:
//RegDst,Jump,Branch,MemRead,MemtoReg,ALUOp,MemWrite,ALUSrc,RegWrite.
unordered_map<string,vector<int>> instructions_map{{"000000",{1,0,0,0,0,2,0,0,1}},{"100011",{0,0,0,1,1,0,0,1,1}},{"101011",{0,0,0,0,0,0,1,1,0}},
						   {"000100",{0,0,1,0,0,1,0,0,0}},{"000010",{0,1,0,0,0,0,0,0,0}},{"001000",{0,0,0,0,0,-1,0,1,1}},
						   {"000101",{0,0,1,0,0,-1,0,0,0}},{"000011",{0,1,0,0,0,-1,0,0,0}}};


//Mapping between the function fields of R-type instructions along with their ALU_control signal values and mapping between the opcodes of I,J type instructions along with their ALU_control signal values.
unordered_map<string,int> R_control{{"100000",2},{"100010",3},{"101010",4},{"000000",5}};          //The instructions are in order add,sub,slt,sll.
unordered_map<string,int> I_J_control{{"100011",2},{"101011",2},{"001000",2},                      //The instructions are in order lw,sw,addi,beq,bne,jal.
			              {"000100",3},{"000101",3},{"000011",7}};


int to_decimal(string);                                                   //Returns the decimal equivalent of the given binary string.
int find_instruction_type(string);                                        //Assigns a number to each instruction based off the opcode of that instruction.
void processor_execute(int,long long int,long long int,int);              //Pipeline processor.
void print_output(string,int,int,long long int,long long int);            //Prints appropriate output based off the program executed.
void perform_ALU_operation(int &,int,int,string,long long int&,int &,long long int,int ALU_control);               //Performs the necessary ALU operation
int find_1s_complement(string);                                           //Returns the 1's complement of the given binary string.


//A data type to store the states of the 5 stages of execution (IF,ID,EX,MEM,WB) of a MIPS instruction.
typedef struct states
{
	vector<int> phase; 
	public:
		states()
		{
			phase={1,1,1,1,1};
		}
		int get_phase(int index)
		{
			return phase[index];
		}
		void set_phase(int index,int num)
		{
			phase[index]=num;
		}
}states;


//Class for IF/ID pipeline.
class IF_ID
{
	string instruction;
	long long int pc;
	int is_empty;
	public:
		IF_ID():is_empty(1)
		{}
		IF_ID(string a,long long int b,int c)
		{
			instruction=a;
			pc=b;
			is_empty=c;
		}
		string get_instruction()
		{
			return instruction;
		}
		long long int get_pc()
		{
			return pc;
		}
		int get_is_empty()
		{
			return is_empty;
		}
};

//Class for ID/EX pipeline.
class ID_EX                                      //ID/EX pipeline register.
{
	string instruction;
	long long int pc;
	int is_empty;
	int instruction_type;
	int rs_index;
	int rt_index;
	int immediate;
	int rd_index;
	vector<int> control_sig;
	public:
		ID_EX():is_empty(1)
		{}
		ID_EX(string a,long long int b,int c,int d,int e,int f,int g,int h,vector<int> k)
		{
			instruction=a;
			pc=b;
			is_empty=c;
			instruction_type=d;
			rs_index=e;
			rt_index=f;
			immediate=g;
			rd_index=h;
			control_sig=k;
		}
		string get_instruction()
		{
			return instruction;
		}
		long long int get_pc()
		{
			return pc;
		}
		int get_is_empty()
		{
			return is_empty;
		}
		int get_instruction_type()
		{
			return instruction_type;
		}
		int get_rs_index()
		{
			return rs_index;
		}
		int get_rt_index()
		{
			return rt_index;
		}
		int get_rd_index()
		{
			return rd_index;
		}
		int get_immediate()
		{
			return immediate;
		}
		vector<int> get_control_sig()
		{
			return control_sig;
		}
};


//Class for EX/MEM pipeline.
class EX_MEM
{
	int is_empty;
	int instruction_type;
	int rs_index;
	int rt_index;
	int immediate;
	int rd_index;
	vector<int> control_sig;
	long long int ALU_result;
	public: 
		EX_MEM()
		{
			is_empty=1;
		}
		EX_MEM(int c,int d,int e,int f,int g,int h,vector<int> k,long long int l)
		{
			is_empty=c;
			instruction_type=d;
			rs_index=e;
			rt_index=f;
			immediate=g;
			rd_index=h;
			control_sig=k;
			ALU_result=l;
		}
		int get_is_empty()
		{
			return is_empty;
		}
		int get_instruction_type()
		{
			return instruction_type;
		}
		int get_rs_index()
		{
			return rs_index;
		}
		int get_rt_index()
		{
			return rt_index;
		}
		int get_rd_index()
		{
			return rd_index;
		}
		int get_immediate()
		{
			return immediate;
		}
		vector<int> get_control_sig()
		{
			return control_sig;
		}
		long long int get_ALU_result()
		{
			return ALU_result;
		}
};

//A class for MEM_WB pipeline.
class MEM_WB
{
	int is_empty;
	int instruction_type;
	int rd_index;
	vector<int> control_sig;
	long long int ALU_result;
	int read_data;
	public:
		MEM_WB():is_empty(1)
		{
		}
		MEM_WB(int c,int d,int h,vector<int> k,long long int l,int p)
		{
			is_empty=c;
			instruction_type=d;
			rd_index=h;
			control_sig=k;
			ALU_result=l;
			read_data=p;
		}
		//Getters
		int get_is_empty()
		{
			return is_empty;
		}
		int get_instruction_type()
		{
			return instruction_type;
		}
		int get_rd_index()
		{
			return rd_index;
		}
		vector<int> get_control_sig()
		{
			return control_sig;
		}
		long long int get_ALU_result()
		{
			return ALU_result;
		}
		int get_read_data()
		{
			return read_data;
		}
};


int main()
{
	vector<string> filename{"Factorial.txt","Insertion_sort.txt"};
	for(int i=0;i<NUM_OF_TRIES;i++)
	{
		int choice;
		int n;
		long long int input_add;
		long long int output_add;

		cout << "1 - Factorial Calculation" << endl;
		cout << "2 - Sorting" << endl;
		cout << "3 - Exit" << endl;
		cout << "Enter choice: " ;
		cin >> choice;

		if(choice==1)
		{
			cout << "Enter the number you want to calculate factorial for: " ;
		}
		else if(choice==2)
		{
			cout << "Enter the number of integers you want to sort: ";
		}
		else
		{
			cout << "Exiting" << endl;
			break;
		}
		cin >> n;

		//Loading the $t1 register with the number of integers taken from user input as user input.
		registers[9]=n;
		if(choice==1)
		{
			registers[9]=1;
		}

		cout << "Enter input address: " ;
		cin >> input_add;
		registers[10]=input_add;                                  //Loading the $t2 register with input address.

		cout << "Enter output address: " ;
		cin >> output_add;
		registers[11]=output_add;                                 //Loading the $t3 register with the output address.

		//Loading the data memory with the user inputs.
		if(choice==1)
		{
			d_mem[(input_add-D_STARTING_ADDRESS)/4]=n;
		}
		else if(choice==2)
		{
			cout << "Enter the integers you want to sort:" << endl;
			int elem;
			for(int i=0;i<n;i++)
			{
				cin >> elem;
				d_mem[((input_add-D_STARTING_ADDRESS)/4)+i]=elem;
			}
		}

		//Copying all the instructions from the required '.txt' file into the instruction memory.
		ifstream input(filename[choice-1]);
		string line;
		while(input.eof()==0)
		{
			getline(input >> ws,line);
			i_mem.push_back(line);
		}
		input.close();
		cout << "I_mem.size(): " << i_mem.size() << endl;

		//Executing the processor.
		processor_execute(n,input_add,output_add,choice);

		i_mem.clear();                                            //Clearing the instruction memory.
		std::fill(d_mem.begin(),d_mem.end(),0);                   //Clearing the Data memory by resetting all the memory spaces to zero.
		std::fill(registers.begin(),registers.end(),0);           //Resetting the values of all the registers to zero.
		PC=I_STARTING_ADDRESS;                                    //Making the PC point back to the starting address of the instruction memory.
		clock_cycle=1;                                            //Resetting the clock cycle counter.
	}
	return 0;
}



void processor_execute(int n,long long int input_add,long long int output_add,int choice)
{
	//Making the 4 pipeline registers along with 4 other empty pipeline registers.
	IF_ID pipeline1,empty1;
	ID_EX pipeline2,empty2;
	EX_MEM pipeline3,empty3;
	MEM_WB pipeline4,empty4;

	//Creating an instance of the class phases.
	states phases;
	int size=i_mem.size();

	while(true)
	{
		//Getting the states of the 5 stages which decide whether that stage will get executed or not.
		int if_state=phases.get_phase(0);
		int id_state=phases.get_phase(1);
		int ex_state=phases.get_phase(2);
		int mem_state=phases.get_phase(3);
		int wb_state=phases.get_phase(4);

		//Printing the clock cycle only if at least one of the 5 stages is executed
		if(mem_state!=0)
		{
			cout << endl;
			cout << endl;
			cout << endl;
			cout << "CLOCK CYCLE: " << clock_cycle << endl;
			clock_cycle++;
		}


		//***************************************WRITEBACK***************************************************
		if(wb_state==5)
		{
			if(mem_state==0)
			{
				phases.set_phase(4,0);
				break;
			}
			if(!pipeline4.get_is_empty())                     //Executing the Write back stage only if the MEM/WB pipeline is not empty.
			{
				cout << "***************************WRITEBACK PHASE**************************" << endl;
				if(pipeline4.get_control_sig()[8]==1 && pipeline4.get_rd_index()!=0)              //If the RegWrite signal of the MEM/WB pipeline is 1 and the destination index is not zero.
				{
					cout << "Writing into the register with index: " << pipeline4.get_rd_index() << endl;
					int write_into_reg=pipeline4.get_ALU_result();
					if(pipeline4.get_control_sig()[4]==1)                      //If the MemtoReg signal is 1, the value written into register is the read data from memory, else it is the ALU result.
					{
						write_into_reg=pipeline4.get_read_data();
					}
					registers[pipeline4.get_rd_index()]=write_into_reg;        //Writing into the register.
					cout << "The value being written back: " << registers[pipeline4.get_rd_index()] << endl;
				}

				cout << "Printing all the regsiters" << endl;
				for(int i=0;i<32;i++)
				{
					cout << registers[i] << " ";
				}
				cout << endl;

				cout << "********************************************************************" << endl;

				if(pipeline1.get_is_empty() && pipeline2.get_is_empty() && pipeline3.get_is_empty() && phases.get_phase(0)==0)
				{
					break;
				}
			}
			else if(pipeline1.get_is_empty() && pipeline2.get_is_empty() && pipeline3.get_is_empty() && phases.get_phase(0)==0)		//If the PC is pointing to the instruction after the last MIPS instruction and the other 3 pipeline registers are empty, then we simply break out of the loop as this break is executed only when pipeline4 is also empty.
			{
				break;
			}
		}	
		else
		{
			phases.set_phase(4,++wb_state);
		}

		//***************************************MEMORY***************************************************
		MEM_WB tmp4;
		if(mem_state==4)
		{
			if(ex_state==0)
			{
				phases.set_phase(3,0);
				pipeline4=empty4;
				continue;
			}
			if(!pipeline3.get_is_empty())                     //Executing the Memory access stage only if the EX/MEM pipeline is not empty.
			{
				cout << "***************************MEMORY PHASE*****************************" << endl;
				int curr_read_data=0;
				if(pipeline3.get_control_sig()[6]==1)     //Writing into memory if the MemWrite signal is 1.
				{
					int write_val_back=registers[pipeline3.get_rt_index()];
					if(!pipeline4.get_is_empty() && pipeline4.get_control_sig()[8]==1 && (pipeline4.get_rd_index()==pipeline3.get_rt_index()))                  //If the MEM/WB pipeline is not empty and the RegWrite control signal is 1 and the destination register of the MEM/WB pipeline is the same as that of the rt index of the EX/MEM pipeline.
					{
						//The value being written back is the ALU result if the MemtoReg is 0, else it writes back the read data from memory.
						write_val_back=pipeline4.get_ALU_result();
						if(pipeline4.get_control_sig()[4]==1)
						{
							write_val_back=pipeline4.get_read_data();
						}
					}
					//Writing the value into memory.
					d_mem[(pipeline3.get_ALU_result()-D_STARTING_ADDRESS)/4]=write_val_back;
					cout << "Writing into memory, the value: " << registers[pipeline3.get_rt_index()] << endl;
				}
				if(pipeline3.get_control_sig()[3]==1)     //Reading from memory if the MemRead signal is 1.
				{
					curr_read_data=d_mem[(pipeline3.get_ALU_result()-D_STARTING_ADDRESS)/4];
					cout << "Reading from memory, the value: " << d_mem[(pipeline3.get_ALU_result()-D_STARTING_ADDRESS)/4];
				}
				cout << "Priting the data in the memory location" << endl;
				for(int i=0;i<registers[9];i++)
				{
					cout << d_mem[((output_add-D_STARTING_ADDRESS)/4)+i] << " ";
				}
				cout << endl;

				//Storing the new results of the stage in a temporary pipeline.
				MEM_WB tmp(pipeline3.get_is_empty(),pipeline3.get_instruction_type(),pipeline3.get_rd_index(),pipeline3.get_control_sig(),pipeline3.get_ALU_result(),curr_read_data);
				tmp4=tmp;
				cout << "********************************************************************" << endl;
			}
			else if(pipeline1.get_is_empty() && pipeline2.get_is_empty() && phases.get_phase(0)==0)
			{
				phases.set_phase(3,0);
			}
		}
		else
		{
			phases.set_phase(3,++mem_state);
		}
		

		//***************************************EXECUTE***************************************************
		EX_MEM tmp3;
		if(ex_state==3)
		{
			if(id_state==0)
			{
				phases.set_phase(2,0);
				pipeline3=empty3;
				pipeline4=tmp4;
				continue;
			}
			if(!pipeline2.get_is_empty())                     //Performing the Execute phase only if the ID/EX pipeline is not empty.
			{
				cout << "***************************EXECUTE PHASE****************************" << endl;
				//getting all the information of the ID/EX pipeline.
				string curr_instruction=pipeline2.get_instruction();
				long long int curr_pc=pipeline2.get_pc();
				int curr_is_empty=pipeline2.get_is_empty();
				int curr_instruction_type=pipeline2.get_instruction_type();
				int curr_rs_index=pipeline2.get_rs_index();
				int curr_rt_index=pipeline2.get_rt_index();
				int curr_immediate=pipeline2.get_immediate();
				int curr_rd_index=pipeline2.get_rd_index();
				int src1=registers[curr_rs_index];
				int src2=registers[curr_rt_index];
				vector<int> curr_control_sig=pipeline2.get_control_sig();


				//Checking for hazards with the MEM/WB pipeline only if it is not empty, the instruction in the MEM/WB is not a J,Jal,Jr instruction and the RegWrite signal is 1.
				if(!pipeline4.get_is_empty() && (curr_instruction_type==4 || curr_instruction_type==5) && pipeline4.get_control_sig()[8]==1)
				{
					cout << "Checking for dependencies using the MEM/WB pipeline register" << endl;
					int forwarded_val=pipeline4.get_ALU_result();
					if(pipeline4.get_control_sig()[4]==1)                      //If the MemtoReg signal is 1, then forwarded value will be read data and not ALU result.
					{
						forwarded_val=pipeline4.get_read_data();
					}

					if(pipeline4.get_rd_index()==curr_rs_index)                //If the destination register of the MEM/WB pipeline matches with rs of the ID/EX pipeline, then we forward the value.
					{
						cout << "Data hazard detected, can be resolved by forwarding." << endl;
						src1=forwarded_val;
					}
					if(pipeline4.get_rd_index()==curr_rt_index)                //If the destination register of the MEM/WB pipeline matches with rt of the ID/EX pipeline, then we forward the value.
					{
						cout << "Data hazard detected, can be resolved by forwarding." << endl;
						src2=forwarded_val;
					}
				}
				//Checking for hazards with the EX/MEM pipeline only if it is not empty, the instruction in EX/MEM is not a J,Jal,Jr.
				if(!pipeline3.get_is_empty() && (curr_instruction_type==4 || curr_instruction_type==5))
				{
					cout << "Checking for dependencies using the EX/MEM pipeline register" << endl;
					int prev_instruction_type=pipeline3.get_instruction_type();
					int prev_RegWrite=pipeline3.get_control_sig()[8];
					if(prev_instruction_type==5 && prev_RegWrite)              //Checking for dependencies if the instruction of EX/MEM pipeline is not a beq or a bne and RegWrite signal is high(1).
					{
						int prev_dst=pipeline3.get_rd_index();
						//If the source registers of the ALU match with the destination registers of the EX/MEM pipeline, then we forward the value.
						if(prev_dst==curr_rs_index)
						{
							cout << "Data hazard detected, can be resolved by forwarding." << endl;
							src1=pipeline3.get_ALU_result();
						}
						if(prev_dst==curr_rt_index)
						{
							cout << "Data hazard detected, can be resolved by forwarding." << endl;
							src2=pipeline3.get_ALU_result();
						}
					}
					if(pipeline3.get_control_sig()[3]==1 && prev_RegWrite)     //Checking for dependencies if the MemRead signal is high(1) and RegWrite signal is also high(1).
					{
						if(pipeline3.get_rd_index()==curr_rs_index || pipeline3.get_rd_index()==curr_rt_index)      //If the destination register of the EX/MEM pipeline matches with either of the source registers of the ID/EX register then there is a RAW hazard. Hence stalling is used.
						{
							cout << "Load hazard detected, can be resolved only by stalling." << endl;
							pipeline3=empty3;
							pipeline4=tmp4;
							continue;
						}
					}
				}

				if(pipeline2.get_control_sig()[7]==1)               //If the ALUSrc signal is high, then the src2 of ALU is the Immediate value.
				{
					src2=curr_immediate;
				}

				//Getting the ALU_control of the current instruction using the opcode or the function field and the control maps.
				int ALU_control=0;
				string key=curr_instruction.substr(0,6);
				unordered_map<string,int> mp=I_J_control;
				if(curr_control_sig[5]==2)
				{
					key=curr_instruction.substr(26,6);
					mp=R_control;
				}
				if(mp.find(key)!=mp.end())
				{
					ALU_control=mp[key];
				}

				int Zero=0;
				int fetch_new=0;
				long long int curr_ALU_result=0;

				//Performing the ALU operation.
				perform_ALU_operation(fetch_new,src1,src2,curr_instruction,curr_ALU_result,Zero,curr_pc,ALU_control);

				//Storing the results of the Execute phase onto a temporary register.
				EX_MEM tmp(curr_is_empty,curr_instruction_type,curr_rs_index,curr_rt_index,curr_immediate,curr_rd_index,curr_control_sig,curr_ALU_result);
				tmp3=tmp;
				cout << "********************************************************************" << endl;

				if(fetch_new)                             //'fetch_new' being equal to 1 indicates that there is a branch and so the ID,IF instructions have to be flushed.
				{
					cout << "Branch detected. Flushing the ID,IF instructions" << endl;
					PC=curr_pc+to_decimal(curr_instruction.substr(16,16))*4;
					pipeline1=empty1;
					pipeline2=empty2;
					pipeline3=tmp3;
					pipeline4=tmp4;
					continue;
				}
			}
			else if(pipeline1.get_is_empty() && phases.get_phase(0)==0)
			{
				continue;
			}
		}
		else
		{
			phases.set_phase(2,++ex_state);
		}


		//***************************************INSTRUCTION DECODE***************************************************
		ID_EX tmp2;
		if(id_state==2)
		{
			if(if_state==0)
			{
				phases.set_phase(1,0);
				pipeline2=empty2;
				pipeline3=tmp3;
				continue;
			}
			if(!pipeline1.get_is_empty())                     //Executing the Decode stage only if the IF/ID pipeline is not empty.
			{
				cout << "***************************INSTRUCTION DECODE***********************" << endl;

				//Getting all the information of the IF/ID pipeline.
				string curr_instruction=pipeline1.get_instruction();
				int curr_is_empty=0;
				int curr_instruction_type=find_instruction_type(curr_instruction);
				int curr_rs_index=to_decimal(curr_instruction.substr(6,5));
				int curr_rt_index=to_decimal(curr_instruction.substr(11,5));
				int curr_immediate_value=to_decimal(curr_instruction.substr(16,16));
				if(curr_instruction[16]==1)               //If the immediate field has the most significant bit as 1, then the immediate value will be a negative number.
				{
					curr_immediate_value=find_1s_complement(curr_instruction.substr(16,16));
				}
				int curr_rd_index=to_decimal(curr_instruction.substr(11,5));
				vector<int> signals=instructions_map[curr_instruction.substr(0,6)];
				long long int curr_pc=pipeline1.get_pc();

				//Changing the destination register based off whether the RegDst signal is high or not.
				if(signals[0]==1)
				{
					curr_rd_index=to_decimal(curr_instruction.substr(16,5));
				}

				cout << "Instruction: " << curr_instruction << endl;
				cout << "PC of this instruction: " << curr_pc-4 << endl;
				cout << "rs_index: " << curr_rs_index << endl;
				cout << "rt_index: " << curr_rt_index << endl;
				cout << "rd_index: " << curr_rd_index << endl;
				cout << "Immediate: " << curr_immediate_value << endl;
				cout << "The instruction_type: " << curr_instruction_type << endl;

				//Storing the results of the ID stage onto a temporary register.
				ID_EX tmp(curr_instruction,curr_pc,curr_is_empty,curr_instruction_type,curr_rs_index,curr_rt_index,curr_immediate_value,curr_rd_index,signals);
				tmp2=tmp;
				cout << "********************************************************************" << endl;

				PC+=4;
				if(curr_instruction_type==1 || curr_instruction_type==2 || curr_instruction_type==3)        //If the instruction is either a J,Jal,Jr then the correct instruction should be fetched in the next clock cycle and the IF of this clock cycle should be flushed.
				{
					pipeline1=empty1;
					pipeline2=tmp2;
					pipeline3=tmp3;
					pipeline4=tmp4;
					//Updating the PC appropriately based on whether the instruction is a J,Jal or Jr.
					PC=registers[31];
					if(curr_instruction_type!=3)
					{
						PC=to_decimal(curr_instruction.substr(6,26))*4;
					}
					continue;
				}
			}
		}
		else
		{
			phases.set_phase(1,++id_state);
		}

		//***************************************INSTRUCTION FETCH***************************************************
		IF_ID tmp1;                                               //Temporary pipeline register to hold the results of the IF stage.
		if(if_state==1)
		{
			cout << "***************************INSTRUCTION FETCH************************" << endl;
			int index=(::PC-I_STARTING_ADDRESS)/4;            //Fetching the instruction from memory.
			if(index==size-1)                                 //This indicates that the pc is pointing to the line after the last MIPS instruction which is just an empty line, thus representing the end of the IF phase for the program.
			{
				phases.set_phase(0,0);
				cout << "PC is " << PC << ", which is pointing outside the range of instruction memory. Hence there is no instruction to fetch." << endl;
				//Making the IF/ID pipeline empty and updating the ID/EX, EX/MEM, MEM/WB pipelines.
				pipeline1=empty1;
				pipeline2=tmp2;
				pipeline3=tmp3;
				pipeline4=tmp4;
				cout << "********************************************************************" << endl;
				continue;
			}
			string p1_instruction=i_mem[index];
			long long int p1_pc=PC+4;
			cout << "Instruction: " << p1_instruction << endl;
			cout << "PC: " << PC << endl;
			IF_ID tmp(p1_instruction,p1_pc,0);
			tmp1=tmp;
			cout << "****************************************************************************" << endl;
		}
		else
		{
			phases.set_phase(0,++if_state);
		}

		cout << "Pipelines are being updated" << endl;
		//Updating all the pipelines to the results of the execution of each stage.
		pipeline1=tmp1;
		pipeline2=tmp2;
		pipeline3=tmp3;
		pipeline4=tmp4;
	}
	cout << "The final clock cycle of the program is: " << clock_cycle-1 << endl;
	cout << "Finished Execution of program." << endl;

	cout << "The final output of ";
	string msg="sorting program is: ";
	if(choice==1)
	{
		msg="factorial program is: ";
	}
	print_output(msg,choice,n,input_add,output_add);
	cout << "The total number of clock cycle taken: " << clock_cycle-1 << endl;
	cout << endl;
	cout << endl;
	cout << endl;
	cout << endl;
}


void perform_ALU_operation(int &fetch_new,int src1,int src2,string curr_instruction,long long int &curr_ALU_result,int &Zero,long long int curr_pc,int ALU_control)
{
	switch(ALU_control)
	{
		case 2: //Performing either of add,sw,lw,addi.
			curr_ALU_result=src1+src2;
			break;
		case 3: //Performing either of sub,beq,bne.
			curr_ALU_result=src1-src2;
			Zero=(curr_ALU_result==0)?1:0;
			if((Zero==1 && curr_instruction.substr(0,6)=="000100") || (Zero!=1 && curr_instruction.substr(0,6)=="000101"))
			{
				fetch_new=1;
			}
			break;
		case 4: //Performing slt.
			curr_ALU_result=0;
			if(src1-src2<0)
			{
				curr_ALU_result=1;
			}
			break;
		case 5: //Performing sll.
			{
				int shamt=to_decimal(curr_instruction.substr(21,5));
				int power=1;
				while(shamt--)
				{
					power*=2;
				}
				curr_ALU_result=power*src2;
			}
			break;
		case 7: //Performing jr by updating the value of $ra register.
			curr_ALU_result=curr_pc;
			registers[31]=curr_pc;
			break;
		default:
			break;
	}
}

//Prints output based off the program being executed.
void print_output(string msg,int choice,int n,long long int input_add,long long int output_add)
{
	cout << msg;
	if(choice==2)
	{
		for(int i=0;i<n;i++)
		{
			cout << d_mem[((output_add-D_STARTING_ADDRESS)/4)+i] << " " ;
		}
	}
	else if(choice==1)
	{
		cout << d_mem[(output_add-D_STARTING_ADDRESS)/4] ;
	}
	cout << endl;
}

//Returns the decimal equivalent of the binary string.
int to_decimal(string binary_string)
{
	int size=binary_string.size();
	int total=0;
	for(int i=0;i<size;i++)
	{
		total*=2;
		if(binary_string[i]=='1')
		{
			total++;
		}
	}
	return total;
}

//Returns the 1's complement of the binary string.
int find_1s_complement(string binary_string)
{
	int size=binary_string.size();
	int total=0;
	for(int i=0;i<size;i++)
	{
		total*=2;
		if(binary_string[i]==0)
		{
			total++;
		}
	}
	return -total;
}

//Returning a number based off the instruction.
int find_instruction_type(string instruction)
{
	string opcode=instruction.substr(0,6);
	if(opcode=="000010")                     //Is j instruction.
	{
		return 1;
	}
	else if(opcode=="000011")                //Is jal instruction
	{
		return 2;
	}
	else if(opcode=="000000" && instruction.substr(26,6)=="001000")   //Is jr instruction
	{
		return 3;
	}
	else if(opcode=="000100" || opcode=="000101")                     //Is beq or bne instruction.
	{
		return 4;
	}
	return 5;                                                         //Any instruction other than the above.
}

