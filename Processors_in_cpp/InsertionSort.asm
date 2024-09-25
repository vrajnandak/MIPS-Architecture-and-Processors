add $t0,$zero,$zero    
#Copying all the inputs into the output address.
copytooutput: slt $s0,$t0,$t1
	beq $s0,$zero,insertionsort   #We branch to insertion sort when i<n. 'i' is $t0 and 'n' is $t1.
	sll $t9,$t0,2
	add $s6,$t2,$t9
	lw $s5,0($s6)
	add $s6,$t3,$t9
	sw $s5,0($s6)
	addi $t0,$t0,1
	j copytooutput

insertionsort: addi $t5,$zero,1       #Initializing i,j,key for insertion sort. 'i' is $t5, 'j' is $t7, and key is $t6.
	add $t6,$zero,$zero 
	add $t7,$zero,$zero

outerloop:
#Checking if i<n. Branch if true.
	slt $s0,$t5,$t1                   
	beq $s0,$zero,finishsort 

	#to access key=arr[i]
	sll $s1,$t5,2
	add $s2,$s1,$t3
	lw $t6,0($s2)

	#j=i-1
	addi $t9,$zero,1
	sub $t7,$t5,$t9

	jal innerloop

	#arr[j+1]=key
	sll $s1,$t7,2
	add $s2,$s1,$t3
	sw $t6,4($s2)

	#i++
	addi $t5,$t5,1
	j outerloop

innerloop:  slt $s0,$t7,$zero
	bne $s0,$zero,goback     #Checking the condition j<0 and goes back to the outer loop if true.

	#storing arr[j] into $s4
	sll $s1,$t7,2
	add $s2,$s1,$t3
	lw $s4,0($s2)
	
	#Checking the second condition, key<arr[j]. Goes back to the outer loop if false
	slt $s0,$t6,$s4
	beq $s0,$zero,goback

	#arr[j+1]=arr[j]
	sw $s4,4($s2)

	#j=j-1
	addi $t9,$zero,1
	sub $t7,$t7,$t9

	j innerloop

goback:
	jr $ra
	
finishsort:
