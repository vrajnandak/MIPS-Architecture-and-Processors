#include<bits/stdc++.h>
using namespace std;

#define STARTING_ADDRESS 4194304

void handle_comments(int *should_continue,int *start,int *end,string line);
string convert_to_5_bit_binary(int decimal_num);
string convert_to_26_bit_binary(string binary_of_llu);
string convert_to_16_bits(long int immediate_value);
void fill_labels(unordered_map<string,string> &labels);

int main()
{
	unordered_map<string,vector<string>> R_umap;         //instruction mapped to {opcodes,shift_amount,function_field,src1,src2,dest}.
	//-1 for src indicates it is a $zero therefore, it is "00000"
	//-2 for src indicates it is a $ra therefore, it is "11111"
	//-3 for shift amount indicates that you have to find out the binary representation of the value into 5 binary bits.
	R_umap["move"]={"000000","-1","2","1","00000","100001"};   
	R_umap["slt"]={"000000","2","3","1","00000","101010"};
	R_umap["add"]={"000000","2","3","1","00000","100000"};
	R_umap["sub"]={"000000","2","3","1","00000","100010"};
	R_umap["sll"]={"000000","-1","2","1","-3","000000"};
	R_umap["jr"]={"000000","1","-1","-1","00000","001000"};

	unordered_map<string,vector<string>> I_umap;                   //instructions mapped to their {opcode,src,dest,imm};
	//tokens[I_umap[][3]] is the immediate value.
	I_umap["beq"]={"000100","1","2","3"}; //tokens[3] is imm
	I_umap["sw"]={"101011","3","1","2"};
	I_umap["addi"]={"001000","2","1","3"};
	I_umap["lw"]={"100011","3","1","2"};
	I_umap["bne"]={"000101","1","2","3"};
	I_umap["li"]={"001001","-1","1","2"};

	unordered_map<string,string> J_umap;  //mapping instruction to opcode only.
	J_umap["jal"]="000011";
	J_umap["j"]="000010";

	unordered_map<string,string> labels;
	fill_labels(labels);

	unordered_map<string,string> registers;
	registers["$zero"]="00000";
	registers["$at"]="00001";
	registers["$v0"]="00010";
	registers["$v1"]="00011";
	registers["$a0"]="00100";
	registers["$a1"]="00101";
	registers["$a2"]="00110";
	registers["$a3"]="00111";
	registers["$t0"]="01000";
	registers["$t1"]="01001";
	registers["$t2"]="01010";
	registers["$t3"]="01011";
	registers["$t4"]="01100";
	registers["$t5"]="01101";
	registers["$t6"]="01110";
	registers["$t7"]="01111";
	registers["$s0"]="10000";
	registers["$s1"]="10001";
	registers["$s2"]="10010";
	registers["$s3"]="10011";
	registers["$s4"]="10100";
	registers["$s5"]="10101";
	registers["$s6"]="10110";
	registers["$s7"]="10111";
	registers["$t8"]="11000";
	registers["$t9"]="11001";
	registers["$k0"]="11010";
	registers["$k1"]="11011";
	registers["$gp"]="11100";
	registers["$sp"]="11101";
	registers["$fp"]="11110";
	registers["$ra"]="11111";

	unordered_map<string,int> lines_for_ori;    //Assuming the immediate values for the instruction ori remain the same. They are nothing but the number of bytes between the first data location(always 0x10010000) and the address of the first byte in the string. The string here refers to the string in 'la $a0,string'.
	lines_for_ori["next_line"]=0;
	lines_for_ori["inp_statement"]=2;
	lines_for_ori["inp_int_statement"]=47;
	lines_for_ori["out_int_statement"]=101;
	lines_for_ori["enter_int"]=157;

	ifstream outf("IMT2022024_IMT2022527_InsertionSort.asm");
	int count=0;
	vector<vector<string>> machine_code;
	int machine_count=STARTING_ADDRESS;
	do
	{
		string line;
		getline(outf,line);
		count++;

		int line_cstart=0; //line comment start
		int line_cend=0;   //line comment end
		int should_continue=0;

		handle_comments(&should_continue,&line_cstart,&line_cend,line);
		if(should_continue==1)   //the line is empty.
		{
			continue;
		}

		string instruction=line.substr(line_cstart,line_cend-line_cstart);
		line_cend -= line_cstart;
		line_cstart -= line_cstart;
		int colon_pos=instruction.find(":");
		int new_start=-1;
		int new_end=line_cend;
		if(colon_pos!=-1)    //it is a label.
		{
			new_start=instruction.find_first_not_of(" \t\r\n",colon_pos+1);
			if(new_start==-1 || new_start==line_cend)     //only label exists in the instruction.
			{
				continue;
			}
			instruction=instruction.substr(new_start,line_cend-new_start);
			line_cstart=0;
			line_cend=line_cend-new_start;
		}
		auto replace_chars={',','(',')'};
		for(auto it:replace_chars)
		{
			replace(instruction.begin(),instruction.end(),it,' ');
		}
		istringstream iss(instruction);
		vector<string> tokens;
		string token;
		while(iss >> token)
		{
			tokens.push_back(token);
		}
		vector<string> mc_line;
		mc_line.push_back(to_string(machine_count));
		if(tokens[0]=="la")
		{
			mc_line.push_back("00111100000000010001000000000001");      //la $a0,address always expands to lui $1,4097 and ori $a0,$1,disp. We are pushing the binary form of the instruction 'lui $1,4097'.
			machine_code.push_back(mc_line);
			machine_count+=4;
			vector<string> next_line;
			next_line.push_back(to_string(machine_count));

			string next_code="001101";     //opcode of ori.
			next_code.append("00001"); 
			next_code.append(registers[tokens[1]]);  
			long int imm=lines_for_ori[tokens[2]];
			next_code.append(convert_to_16_bits(imm));

			next_line.push_back(next_code);
			machine_code.push_back(next_line);
			machine_count+=4;
			continue;
		}
		else if(tokens[0]=="syscall")
		{
			string code="00000000000000000000000000001100";    //syscall always has the code as 0x0000000c
			mc_line.push_back(code);
		}
		else
		{
			string code;
			if(R_umap.find(tokens[0])!=R_umap.end())      //It is an R-Type instruction
			{
				code=R_umap[tokens[0]][0];
				for(int i=1;i<4;i++)
				{
					string val=R_umap[tokens[0]][i];
					string reg=(val=="-1")?"00000":registers[tokens[stoi(val)]];
					code.append(reg);
				}
				for(int i=4;i<6;i++)
				{
					string val=R_umap[tokens[0]][i];
					string shift_or_func=(val=="-3")?convert_to_5_bit_binary(stoi(tokens[3])):val;
					code.append(shift_or_func);
				}
			}
			else if(J_umap.find(tokens[0])!=J_umap.end())     //It is a J-Type instruction
			{
				code.append(J_umap[tokens[0]]);
				code.append(convert_to_26_bit_binary(labels[tokens[1]]));
			}
			else if(I_umap.find(tokens[0])!=I_umap.end())              //It is an I-Type instruction
			{
				code.append(I_umap[tokens[0]][0]);
				for(int i=1;i<3;i++)
				{
					string val=I_umap[tokens[0]][i];
					string reg=(val=="-1")?"00000":registers[tokens[stoi(val)]];
					code.append(reg);
				}
				long int imm;
				if(tokens[0]=="beq" || tokens[0]=="bne")
				{
					string val=tokens[3];
					imm=(((stoi(labels[val])-machine_count)/4)-1); //The immediate field of beq and bne is the number of instructinos that the address is away from pc+4. This assignment gives the required immediate_value
				}
				else
				{
					string val=I_umap[tokens[0]][3];
					imm=stoi(tokens[stoi(val)]);
				}
				code.append(convert_to_16_bits(imm));
			}
			mc_line.push_back(code);
		}
		machine_code.push_back(mc_line);
		machine_count += 4;
	}while(outf.eof()==0);
	outf.close();

	ofstream into_file("SelfGeneratedBinary.txt");
	for(auto it:machine_code)
	{
		//We can also push in the address with the machine_code by doing the following
		//into_file << it[0] << " " << it[1] << endl;
		into_file << it[1] << endl;         //We are pushing only the instruction in binary and not passing the address.
	}
	into_file.close();
	return 0;
}


void handle_comments(int *should_continue,int *start,int *end,string line)
{
	if(line.find(".ascii")!=-1 || line.find(".text")!=-1 || line.find(".data")!=-1)
	{
		*should_continue=1;
		return;
	}
	*end=line.find("#");
	if(*end==-1)
	{
		*end=line.size();
	}
	*start=line.find_first_not_of(" \t\r\n");
	if(*start==-1 || *start==*end)   //implies, it is an empty line or is a commented line.
	{
		*should_continue=1;
	}
}
string convert_to_5_bit_binary(int decimal_num)
{
	bitset<5> binary(decimal_num);
	return binary.to_string();
}


string convert_to_16_bits(long int immediate_value)
{
	bitset<16> binary(immediate_value);
	return binary.to_string();
}

string convert_to_26_bit_binary(string binary_of_llu)
{
	long long int int_binary_of_llu=stoll(binary_of_llu);
	string res=bitset<32>(int_binary_of_llu).to_string();
	return res.substr(4,26);
}

void fill_labels(unordered_map<string,string> &labels)
{
	ifstream out("IMT2022024_IMT2022527_InsertionSort.asm");
	string line;
	long int machine_count=STARTING_ADDRESS;
	int count=0;
	do
	{
		getline(out,line);
		count++;

		int line_cstart=0; //line comment start
		int line_cend=0;   //line comment end
		int should_continue=0; 
		handle_comments(&should_continue,&line_cstart,&line_cend,line);
		if(should_continue==1)   //the line is empty.
		{
			continue;
		}

		string instruction=line.substr(line_cstart,line_cend-line_cstart);
		line_cend -= line_cstart;
		line_cstart -= line_cstart;
		int colon_pos=instruction.find(":");
		int new_start=-1;
		int new_end=line_cend;
		if(colon_pos!=-1)    //it is a label.
		{
			new_start=instruction.find_first_not_of(" \t\r\n",colon_pos+1);
			string label=instruction.substr(line_cstart,colon_pos-line_cstart);
			labels[label]=to_string(machine_count);
			if(new_start==-1 || new_start==line_cend)     //only label exists in the instruction.
			{
				continue;
			}
		}
		if(line.find("la")!=-1)
		{
			machine_count+=4;
		}
		machine_count+=4;
	}while(out.eof()==0);
	out.close();
}

