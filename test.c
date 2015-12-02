#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <othm_hashline.h>

int str_request_cmp(void *hashline_data, void *request_data)
{
	return !(strcmp((char *) hashline_data,
			(char *) request_data));
}

struct othm_hashline *hasline_gen(void)
{
	return malloc(sizeof(struct othm_hashline));
}

void hashline_free(struct othm_hashline *line)
{
	free(line);
}

int main(void)
{
	char *str  = "If you are seeing this, ";
	char *str2 = "you built othm_hashline! ";
	char *str3 = "Yay!\n";
	struct othm_hashline *a = othm_hashline_new(hasline_gen);


	struct othm_request *b = othm_request_new
		(str_request_cmp, str, strlen(str), str);
	struct othm_request *c = othm_request_new
		(str_request_cmp, str2, strlen(str2), str2);
	struct othm_request *d = othm_request_new
		(str_request_cmp, str3, strlen(str3), str3);

	othm_hashline_add(a, b);
	othm_hashline_add(a, d);


        if (othm_hashline_get(a, b))
		printf(str);
        if (!othm_hashline_get(a, c))
		printf(str2);
	if (othm_hashline_get(a, d))
		printf(str3);

	othm_hashline_remove(a, b);
	if(!othm_hashline_get(a, b))
		printf("It is also fully functional!\n");
	othm_hashline_free(a, hashline_free);
	free(c);
	free(b);
	free(d);
	return 0;
}
