//Non-pipelined processor simulation using C++. The following is capable of executing only a few MIPS instruction and hence can execute any MIPS program using those MIPS instructions.

#include<iostream>
#include<fstream>
#include<unordered_map>
#include<vector>
using namespace std;

#define I_STARTING_ADDRESS 4194304               //Macro for defining the starting address of Instruction Memory.
#define D_STARTING_ADDRESS 268500992             //Macro for defining the starting address of Data Memory.
#define REG_NUMBERS 32                           //Macro for the total number of registers.
#define MEMORY_SPACE 100                         //Macro for defining the memory size.

long long int clock_cycle=1;                     //Counter for keeping track of the current cycle number.
long long int PC=I_STARTING_ADDRESS;             //Program Counter, loaded with the address of the first instruction to be executed.

//Control Signals used by the processor.
int RegDst;
int Jump;
int Branch;
int MemRead;
int MemtoReg;
int ALUOp;
int MemWrite;
int ALUSrc;
int RegWrite;

vector<string> i_mem;                            //Instruction Memory.
vector<int> d_mem(MEMORY_SPACE,0);               //Data Memory.

vector<int> reg(REG_NUMBERS,0);                  //Registers are maintained as a vector. Any register can be accessed using it's register number.

//The use_* functions return the output of the corresponding MUX using the required control signal.
int use_RegDst(string);
int use_ALUSrc(int,int);
long long int use_Jump(int,long long int);
long long int use_MemtoReg(int,long long int);
long long int use_PCSrc(long long int,long long int,int,string);

void print_clk();                                                         //Prints the current clock cycle number.
int to_decimal(string);                                                   //Converts the given binary string into decimal equivalent.
void generate_signals(string);                                            //Generates all the control signals for the given instruction.
int find_1s_complement(string);                                           //Returns the 1s complement of the given string.
long long int generate_PC(string,int,int);                                //Generates the next value of PC based off instruction,Zero,Jump,Branch.
void processor_execute(int,long long int,long long int,int);              //Simulates the working of a MIPS processor for a few MIPS instructions.
void print_output(string,int,int,long long int,long long int);            //Prints the output message based on the choice.
void perform_ALU_operation(long long int&,int&,int,int,string);           //Performs the required ALU operation based off control signals.

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

int main()
{
	vector<string> filename{"Factorial.txt","Insertion_sort.txt"};
	for(int i=0;i<2;i++)
	{
		int choice;
		int n;
		long long int input_add;
		long long int output_add;

		cout << "1 - Factorial Calculation" << endl;
		cout << "2 - Sorting" << endl;
		cout << "Enter choice: " ;
		cin >> choice;

		if(choice==1)
		{
			cout << "Enter the number to calculate factorial for: " ;
			cin >> n;
			
			reg[9]=1;                                         //Loading the $t1 register with the value n.

			//*****There must be enough gap between the input and output addresses if the MIPS program copies the user input in the input address to the output address before sorting***********
			cout << "Enter the input address: " ;
			cin >> input_add;
			reg[10]=input_add;                                //Loading the $t2 register with the input addrss.

			cout << "Enter the output address: " ;
			cin >> output_add;
			reg[11]=output_add;                               //Loading the $t3 register with the output address.
			d_mem[(input_add-D_STARTING_ADDRESS)/4]=n;
		}
		else if(choice==2)
		{
			cout << "Enter the number of integers to be sorted: " ;
			cin >> n;
			reg[9]=n;                                         //Loading the $t1 register with value n.

			//*****There must be enough gap between the input and output addresses if the MIPS program copies the user input in the input address to the output address before sorting***********
			cout << "Enter the input address: " ;
			cin >> input_add;
			reg[10]=input_add;                                //Loading the $t2 register with the input address.

			cout << "Enter the output address: " ;
			cin >> output_add;
			reg[11]=output_add;                               //Loading the $t3 register with the output address.

			cout << "Enter the numbers: " << endl;
			int elem;
			for(int i=0;i<n;i++)
			{
				cin >> elem;
				d_mem[((input_add-D_STARTING_ADDRESS)/4)+i]=elem;
			}

		}

		//Copying all the instructions from the appropriate txt file into the instruction memory.
		ifstream input(filename[choice-1]);
		string line;
		while(input.eof()==0)
		{
			getline(input >> ws,line);
			i_mem.push_back(line);
		}
		input.close();

		//Executing the processor.
		processor_execute(n,input_add,output_add,choice);

		i_mem.clear();                                            //Clearing the instruction memory.
		std::fill(d_mem.begin(),d_mem.end(),0);                   //Clearing the Data memory by resetting all the memory spaces to zero.
		std::fill(reg.begin(),reg.end(),0);                       //Resetting the values of all the registers to zero.
		PC=I_STARTING_ADDRESS;
		clock_cycle=1;
	}

	return 0;
}

void processor_execute(int n,long long input_add,long long int output_add,int choice)
{
	int counter=1;                                                    //Keeps track of the total number of instructions executed.
	int size=i_mem.size();

	while(true)
	{
		//***********************INSTRUCTION FETCH PHASE(IF)********************************
		int index=(PC-I_STARTING_ADDRESS)/4;
		if(index==size-1)                                         //index having value size-1 indicates that the end of the MIPS program.
		{
			break;
		}
		print_clk();
		cout << "This is the " << counter << "th instruction to be executed" << endl;
		counter++;

		cout << "The index of the instruction in the memory is: " << index << endl;
		string instruction=i_mem[index];                          //Accessing the instruction from the memory.


		//***********************INSTRUCTION DECODE PHASE(ID)*******************************
		print_clk();
		//Filling the read register ports of the RegFile with the appropriate reg numbers.
		int reg1=to_decimal(instruction.substr(6,5));
		int reg2=to_decimal(instruction.substr(11,5));
		generate_signals(instruction);                            //Generating the control signals based off the instruction.
		int index_write_reg=use_RegDst(instruction);              //Filling the write register port of regfile with the appropriate reg number.
		cout << "The 2 read ports and the write port of the RegFile are: " << reg1 << " " << reg2 << " " << index_write_reg << " respectively" << endl;
		int sign_extend=to_decimal(instruction.substr(16,16));     //Generating the immediate value of the instruction. We don't have to explicitly add zero's because this is implicitly taken care of by C++. The int data type in C++ uses 4 bytes(i.e., 32 bits) and all the bits are zero by default and so there is no need for sign extending the immediate value.
		if(instruction[16]==1)
		{
			sign_extend=find_1s_complement(instruction.substr(16,16));
		}
		cout << "Sign extend value: " << sign_extend << endl;


		//***********************ALU EXECUTE PHASE(EX)**************************************
		print_clk();
		//Loading values of the 2 sources of ALU.
		int alu_src1=reg[reg1];                                   //Simply loading in the 'rs' into src1.
		int alu_src2=use_ALUSrc(reg[reg2],sign_extend);           //Loading the output of the MUX based off the ALUSrc control signal.
		long long int ALU_result;
		int Zero;
		perform_ALU_operation(ALU_result,Zero,alu_src1,alu_src2,instruction);              //Performing the necessary ALU operation.
		cout << "src1: " << alu_src1 << ", src2: " << alu_src2 << ", ALU result: " << ALU_result << endl;
		cout << "Current PC: " << PC << endl;
		PC=generate_PC(instruction,sign_extend,Zero);             //Generating the address of the next instruction to be executed.
		cout << "Next PC: " << PC << endl;


		//***********************MEMORY ACCESS PHASE(MEM)***********************************
		print_clk();
		int write_data=reg[reg2];                                 //Filling the write data port of the data memory.
		long long int address=ALU_result;                                   //Filling the address port of the data memory.
		cout << "Write data port of memory: " << write_data << endl;
		cout << "Address port of memory: " << address << endl;
		int read_data=0;                                          
		if(MemWrite==1)                                           //Writing into memory.
		{
			d_mem[(ALU_result-D_STARTING_ADDRESS)/4]=write_data;
		}
		if(MemRead==1)                                            //Reading from memory.
		{
			read_data=d_mem[(ALU_result-D_STARTING_ADDRESS)/4];
			cout << "Data read from memory: " << read_data << endl;
		}


		//***********************WRITE BACK PHASE(WB)***************************************
		print_clk();
		int write_val_back=use_MemtoReg(read_data,ALU_result);    //Loading the write data port of the RegFile with the output of the mux.
		if(RegWrite==1)
		{
			if(index_write_reg!=0)
			{
				cout << "Writing into register. Value being written is: " << write_val_back << endl;
				reg[index_write_reg]=write_val_back;
			}
		}


		//***********************PRINTING ALL THE REGISTERS AND THE VALUES AT THE OUTPUT ADDRESS*********************
		cout << "Printing all the registers after WB phase" << endl;
		for(int i=0;i<REG_NUMBERS;i++)
		{
			cout << reg[i] << " ";
		}
		cout << endl;
		print_output("Current output address has: ",choice,n,input_add,output_add);
		cout << endl;
		cout << endl;
		cout << endl;
		cout << endl;
	}

	cout << "Finished Execution of program in the new Instruction fetch as there is no instruction to execute." << endl;

	cout << "The final output of " ;
	string msg="sorting program is: ";
	if(choice==1)
	{
		msg="factorial program is: ";
	}
	print_output(msg,choice,n,input_add,output_add);
	cout << "The total number of clock cycles taken: " << clock_cycle-1 << endl;     
	cout << endl;
	cout << endl;
	cout << endl;
	cout << endl;
}

void print_output(string msg,int choice,int n,long long int input_add,long long int output_add)
{
	cout << msg ;
	if(choice==2)
	{
		for(int i=0;i<n;i++)
		{
			cout << d_mem[((output_add-D_STARTING_ADDRESS)/4)+i] << " ";
		}
	}
	else if(choice==1)
	{
		cout << d_mem[(output_add-D_STARTING_ADDRESS)/4] ;
	}
	cout << endl;
}


void print_clk()
{
	cout << "Clock cycle: " << clock_cycle << endl;
	clock_cycle++;
}

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

int find_1s_complement(string binary_string)
{
	int total=0;
	int size=binary_string.size();
	for(int i=0;i<size;i++)
	{
		total*=2;
		if(binary_string[i]==0)
		{
			total++;
		}
	}
	return total;
}


void generate_signals(string instruction)
{
	string key=instruction.substr(0,6);      //key is the opcode of the instruction.

	//Filling in the values of the control signals based off the opcode using the instructions map.
	RegDst=instructions_map[key][0];
	Jump=instructions_map[key][1];
	Branch=instructions_map[key][2];
	MemRead=instructions_map[key][3];
	MemtoReg=instructions_map[key][4];
	ALUOp=instructions_map[key][5];
	MemWrite=instructions_map[key][6];
	ALUSrc=instructions_map[key][7];
	RegWrite=instructions_map[key][8];
}

long long int generate_PC(string instruction,int sign_extend,int Zero)
{
	if(instruction.substr(0,6)=="000000" && instruction.substr(26,6)=="001000")
	{
		return reg[31];
	}
	long long int option1=PC+4;
	long long int option2=PC+4+(4*sign_extend);
	long long int result1=use_PCSrc(option1,option2,Zero,instruction.substr(0,6));

	int option3=to_decimal(instruction.substr(6,26))*4;    //Multiplying by 4 is Eq. to shifting left by 2 and appending the 4 zeroes. We need not sign extend this because this is taken care of automatically by C++ since int is 4 bytes(i.e., 32 bits) and the bits are 0 by default.
	return use_Jump(option3,result1);
}

long long int use_PCSrc(long long int option1,long long int option2,int Zero,string opcode)
{
	if((Branch==1 && Zero==1 && opcode=="000100") || (Branch==1 && Zero==0 && opcode=="000101"))        
	{
		return option2;
	}
	return option1;
}

long long int use_Jump(int option3,long long int result1)
{
	long long int result=(Jump==0?result1:option3);
	return result;
}

int use_RegDst(string instruction)
{
	int index=(RegDst==1)?16:11;
	return to_decimal(instruction.substr(index,5));
}

int use_ALUSrc(int reg2,int sign_extend)
{
	return ((ALUSrc==1)?sign_extend:reg2);
}

void perform_ALU_operation(long long int &ALU_result,int &Zero,int src1,int src2,string instruction)
{
	int ALU_control=0;
	string key;
	unordered_map<string,int> mp;

	if(ALUOp==2)
	{
		key=instruction.substr(26,6);
		mp=R_control;
	}
	else
	{
		key=instruction.substr(0,6);
		mp=I_J_control;
	}

	if(mp.find(key)!=mp.end())
	{
		ALU_control=mp[key];
	}

	Zero=0;
	cout << "ALU_CONTROL: " << ALU_control << endl;
	switch(ALU_control)
	{
		case 0: //There is no need to use ALU.
			break;
		case 2: //Performing either of add,sw,lw,addi.
			ALU_result=src1+src2;
			break;
		case 3: //Performing either of sub,beq,bne.
			ALU_result=src1-src2;
			Zero=(ALU_result==0?1:0);
			break;
		case 4: //Performing slt.
			ALU_result=0;
			if(src1-src2<0)
			{
				ALU_result=1;
			}
			break;
		case 5: //Performing sll.
			{
				int shamt=to_decimal(instruction.substr(21,5));
				int power=1;
				while(shamt--)
				{
					power*=2;
				}
				ALU_result=power*src2;
			}
			break;
		case 7: //Performing jal.
			reg[31]=PC+4;
			break;
	}
}

long long int use_MemtoReg(int read_data,long long int ALU_result)
{
	return (::MemtoReg==1)?read_data:ALU_result;
}
