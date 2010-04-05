/*
   Copyright (c) <2010> <Jo�o Costa>
   Dual licensed under the MIT and GPL licenses.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <my_global.h>
#include <my_sys.h>
#include <mysql.h>
#include <ctype.h>
#include "ta_libmysqludf_ta.h"

extern int getNextSlot(int, int );

/*
   CREATE FUNCTION ta_min RETURNS REAL SONAME 'lib_mysqludf_ta.so';
   DROP FUNCTION ta_min;
 */

typedef struct ta_min_data {
	int current;
	int next_slot;
	double values[];
} ta_min_data;

double minInSlot(int ringSize, double * d)
{
	int i = 0;
	double min = d[0];

	for (i = 0; i < ringSize; i++)
		if (d[i] < min)
			min = d[i];
	return min;
}

DLLEXP my_bool ta_min_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	ta_min_data* data;

	initid->maybe_null = 1;

	if (args->arg_count != 2) {
		strcpy(message, "ta_min() requires two arguments");
		return 1;
	}

	if (args->arg_type[0] == DECIMAL_RESULT)
		args->arg_type[0] = REAL_RESULT;
	else if (args->arg_type[0] != REAL_RESULT) {
		strcpy(message, "ta_min() requires a real");
		return 1;
	}

	if (args->arg_type[1] != INT_RESULT) {
		strcpy(message, "ta_min() requires an integer");
		return 1;
	}

	if (!(data = (ta_min_data *)malloc(sizeof(ta_min_data) + (*(int *)args->args[1]) * sizeof(double)))) {
		strcpy(message, "ta_min() couldn't allocate memory");
		return 1;
	}

	data->current   = 0;
	data->next_slot = 0;

	initid->ptr = (char*)data;
/*
    fprintf(stderr, "Init ta_min %i done\n", (*(int *) args->args[1]));
    fflush(stderr);
 */
	return 0;
}

DLLEXP void ta_min_deinit(UDF_INIT *initid)
{
	free(initid->ptr);
}

DLLEXP double ta_min(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
	ta_min_data *data = (ta_min_data *)initid->ptr;
	int *periods = (int *)args->args[1];

	if (args->args[0] == NULL) {
		if (data->current > 0) {
			strcpy(error, "ta_min() Can't handle NULL values in middle of dataset");
			return 1;
		}
		*is_null = 1;
		return 0.0;
	}

	data->current = data->current + 1;
	data->values[data->next_slot] = *((double*)args->args[0]);
	data->next_slot = getNextSlot(data->next_slot, *periods);

	if (*periods > data->current) {
		*is_null = 1;
		return 0.0;
	} else
		return minInSlot(*periods, data->values );
}
