#include <stdio.h>
#define STACK_SIZE 15
double stack[STACK_SIZE];
int top = 0;

void push(double value) { //Функция push
	if (top >= STACK_SIZE) {
		printf("E: mc went on the defensive\n");
		return;
	}
	stack[top] = value;
	top++;
}
double pop() {
	if (top == 0) {
	     printf("E: stack is empty\n");
	     return 0.0;
	}
	top--;
	return stack[top];
}
int main() {
	char input[20];
	double a,b;

	printf("--Ocean Kernel Calculator--\n");
	printf("enter operator\n");

	while(1) {
	    printf("> ");
	    if (scanf("%s", input) == -1) break;

	    // плюс
	    if (input[0] == '+' && input[1] == '\0') {
		b = pop();
		a = pop();
		push(a + b);
		printf("result: %lf\n", stack[top - 1]);
           } 

	   // минус
           else if (input[0] == '-' && input[1] == '\0') {
                b = pop();
                a = pop();
                push(a-b);
                printf("result: %lf\n", stack[top - 1]);
          }

          // умножение
          else if (input[0] == '*' && input[1] == '\0') {
                b = pop();
                a = pop();
                push(a*b);
                printf("result: %lf\n", stack[top - 1]);
          }

         // деление
         else if (input[0] == '/' && input[1] == '\0') {
                b = pop();
                a = pop();
                if (b == 0.0) {
                    printf("E: you can't divide by zero\n");
                    push(a);
                    push(b);
                } else {
                    push(a / b);
                    printf("result: %lf\n", stack[top - 1]);
                }

        }
        // если это собственно число
        else {
            double value;
            if (sscanf(input, "%lf", &value) == 1) {
                push(value);
            } else {
                printf("E: unknown command\n");
            }
        }
    }
    return 0;
    }