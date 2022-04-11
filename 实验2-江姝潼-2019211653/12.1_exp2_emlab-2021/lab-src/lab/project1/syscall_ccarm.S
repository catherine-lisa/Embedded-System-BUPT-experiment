
    AREA |.text|, CODE, READONLY, ALIGN=2
    THUMB
    REQUIRE8
    PRESERVE8

SYS_system_call    		EQU     0
SYS_fork          		EQU     1
SYS_call_test      		EQU     2

    MACRO                 ; start macro definition
	SYSCALL	$funcname
$funcname PROC	
	EXPORT $funcname
    push {r12, lr} ;			
    mov r12, #SYS_$funcname ;
    svc 0 ;					
    nop	;						
    pop {r12, lr} ;				
    bx lr;
	ENDP
    MEND                         

    ;syscall user function definition
	SYSCALL	system_call
	SYSCALL	call_test
    SYSCALL	fork


	END