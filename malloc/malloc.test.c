#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "testlib.h"
#include "malloc.h"


static void
successful_malloc_returns_non_null_pointer(void)
{
	char *var = malloc(100);

	ASSERT_TRUE("1.successful malloc returns non null pointer", var != NULL);

	free(var);
}

static void
correct_copied_value(void)
{
	char *test_string = "FISOP malloc is working!";

	char *var = malloc(100);

	strcpy(var, test_string);

	ASSERT_TRUE("2.allocated memory should contain the copied value",
	            strcmp(var, test_string) == 0);

	free(var);
}


static void
correct_amount_of_mallocs(void)
{
	struct malloc_stats stats;

	char *var = malloc(100);

	free(var);

	get_stats(&stats);

	ASSERT_TRUE("3.amount of mallocs should be one", stats.mallocs == 1);
}

static void
correct_amount_of_frees(void)
{
	struct malloc_stats stats;

	char *var = malloc(100);

	free(var);

	get_stats(&stats);

	ASSERT_TRUE("4.amount of frees should be one", stats.frees == 1);
}

static void
correct_amount_of_requested_memory(void)
{
	struct malloc_stats stats;

	char *var = malloc(99);

	free(var);

	get_stats(&stats);

	ASSERT_TRUE("5.amount of requested memory should be 100",
	            stats.requested_memory == 100);
}

static void
correct_copied_values(void)
{
	char *test_string = "FISOP malloc is working!";
	char *test_string2 = "FISOP malloc2 is working!";

	char *var = malloc(100);
	strcpy(var, test_string);
	ASSERT_TRUE("6.alocated memory should contain the first copied "
	            "value",
	            strcmp(var, test_string) == 0);

	char *var2 = malloc(200);
	strcpy(var2, test_string2);
	ASSERT_TRUE("7.allocated memory should contain the second "
	            "copied value",
	            strcmp(var2, test_string2) == 0);

	free(var);
	free(var2);
}

static void
malloc_bigger_than_block(void)
{
	char *var = malloc(100000);

	struct region *first_region =
	        (struct region *) ((size_t) var - sizeof(struct region));

	size_t first_block_size = first_region->size + first_region->next->size +
	                          2 * sizeof(struct region);

	ASSERT_TRUE("8.malloc bigger than block creates a MEDIUM block",
	            first_block_size == MEDIUM);

	free(var);
}

static void
not_first_malloc_bigger_than_block(void)
{
	char *var = malloc(100);
	char *var2 = malloc(100000);

	struct region *first_region =
	        (struct region *) ((size_t) var - sizeof(struct region));

	size_t first_block_size = first_region->size + first_region->next->size +
	                          2 * sizeof(struct region);

	size_t second_block_size = first_region->next->next->size +
	                           first_region->next->next->next->size +
	                           2 * sizeof(struct region);

	ASSERT_TRUE("9.second malloc bigger than block generates a MEDIUM "
	            "block after the first SMALL block",
	            first_block_size == SMALL && second_block_size == MEDIUM);

	free(var);
	free(var2);
}

static void
sum_of_malloc_sizes_bigger_than_block(void)
{
	char *var = malloc(1000);
	char *var2 = malloc(1500);

	struct region *first_region =
	        (struct region *) ((size_t) var - sizeof(struct region));
	struct region *var2_region =
	        (struct region *) ((size_t) var2 - sizeof(struct region));

	size_t first_block_size = first_region->size + first_region->next->size +
	                          2 * sizeof(struct region);


	ASSERT_TRUE("10.second malloc bigger than free space generates a SMALL "
	            "block after the first SMALL block (1/2)",
	            first_block_size == SMALL);


	size_t second_block_size = first_region->next->next->size +
	                           first_region->next->next->next->size +
	                           2 * sizeof(struct region);

	ASSERT_TRUE("11.second malloc bigger than free space generates a SMALL "
	            "block after the first SMALL block (2/2)",
	            second_block_size == SMALL);

	free(var);
	free(var2);
}

static void
ptr_to_malloc_returns_correct_position(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(200);

	struct region *first_region =
	        (struct region *) ((size_t) var - sizeof(struct region));


	ASSERT_TRUE("12.two mallocs smaller than the block should be in the "
	            "same block",
	            var + first_region->size + 1 * sizeof(struct region) == var2);


	free(var);
	free(var2);
}

static void
use_freed_region(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);
	size_t var3 = (size_t) malloc(100);

	free(var2);

	size_t var4 = (size_t) malloc(100);

	ASSERT_TRUE("13.malloc of a new region uses previously freed "
	            "region "
	            "instead of a new one",
	            var2 == var4);
}

static void
coalesing_with_previous_region(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);
	size_t var3 = (size_t) malloc(100);

	free(var);
	free(var2);

	size_t var4 = (size_t) malloc(150);

	ASSERT_TRUE("14.a freed region is coalesced with its previous "
	            "region if "
	            "its free",
	            var == var4);
}

static void
coalesing_with_previous_region_also_includes_region_header(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);
	size_t var3 = (size_t) malloc(100);

	free(var);
	free(var2);

	size_t var4 = (size_t) malloc(232);

	ASSERT_TRUE("15.a freed region is coalesced with its previous "
	            "region if "
	            "its free and the region header is also included",
	            var == var4);
}

static void
cant_use_freed_region_if_malloc_size_is_bigger(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);
	size_t var3 = (size_t) malloc(100);

	free(var);
	free(var2);

	size_t var4 = (size_t) malloc(200 + sizeof(struct region) + 1);

	ASSERT_TRUE("16.malloc does not point to freed region if its "
	            "size is "
	            "bigger",
	            var != var4);
}

static void
coalesing_with_following_region(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);
	size_t var3 = (size_t) malloc(100);

	free(var2);
	free(var);

	size_t var4 = (size_t) malloc(150);

	ASSERT_TRUE("17.a freed region is coalesced with its next "
	            "region if "
	            "its free",
	            var == var4);
}

static void
coalesing_with_following_region_also_includes_region_header(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);
	size_t var3 = (size_t) malloc(100);

	free(var2);
	free(var);

	size_t var4 = (size_t) malloc(232);

	ASSERT_TRUE("18.a freed region is coalesced with its next "
	            "region if "
	            "its free and the region header is also included",
	            var == var4);
}

static void
malloc_of_a_small_enough_space_allows_splitting(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);
	size_t var3 = (size_t) malloc(100);

	free(var2);
	free(var);

	size_t var4 = (size_t) malloc(125);

	size_t var5 = (size_t) malloc(30);

	ASSERT_TRUE("19.if a big enough space is found for the next "
	            "malloc after "
	            "splitting then the new malloc will point at that "
	            "space",
	            var5 < var3);
}

static void
malloc_of_size_bigger_than_small_creates_a_bigger_region(void)
{
	size_t var = (size_t) malloc(SMALL);

	struct region *first_region =
	        (struct region *) (var - sizeof(struct region));

	ASSERT_TRUE("20.malloc of size bigger than small creates a bigger "
	            "region",
	            MEDIUM == first_region->size + first_region->next->size +
	                              2 * sizeof(struct region));
}

static void
freeing_all_regions_of_last_block_frees_block()
{
	struct malloc_stats stats;
	char *var1 = malloc(2048 - sizeof(struct region));
	char *var2 = malloc(100);

	free(var2);
	get_stats(&stats);

	ASSERT_TRUE("21. freeing all regions of the last block frees the "
	            "entire block",
	            stats.blocks_counter == 1);
}

static void
freeing_all_regions_of_middle_block_frees_block()
{
	size_t var1 = (size_t) malloc(SMALL - sizeof(struct region));
	size_t var2 = (size_t) malloc(SMALL - sizeof(struct region));
	size_t var3 = (size_t) malloc(100);


	struct region *first_region =
	        (struct region *) (var1 - sizeof(struct region));
	struct region *last_region =
	        (struct region *) (var3 - sizeof(struct region));


	free(var2);

	ASSERT_TRUE("22. freeing all regions of a block in the middle "
	            "frees "
	            "the entire block (1/2)",
	            first_region->next == last_region);


	ASSERT_TRUE("22. freeing all regions of a block in the middle "
	            "frees "
	            "the entire block (2/2)",
	            last_region->prev == first_region);
}

static void
testing_finding_free_regions()
{
	size_t var = (size_t) malloc(200);
	size_t var2 = (size_t) malloc(200);
	size_t var3 = (size_t) malloc(200);
	size_t var4 = (size_t) malloc(100);
	size_t var5 = (size_t) malloc(100);

	free(var2);
	free(var4);

	size_t var_new = (size_t) malloc(50);

	ASSERT_TRUE("23. FIRST FIT: new malloc should use first empty space "
	            "big enough",
	            var_new == var2);

	ASSERT_TRUE("24. BEST FIT: new malloc should use smaller empty space "
	            "big enough",
	            var_new == var4);
}

static void
calloc_returns_non_null_pointer(void)
{
	size_t var = (size_t) calloc(1, 100);

	ASSERT_TRUE("25. calloc returns non null pointer", var != NULL);
}

static void
calloc_initializes_memory_to_zero(void)
{
	char *var = calloc(1, 100);

	int i = 0;
	while (var[i] == 0 && i < 100) {
		i++;
	}

	ASSERT_TRUE("26. calloc initializes memory to zero", i == 100);

	free(var);
}

static void
calloc_correct_copied_value(void)
{
	char *test_string = "FISOP malloc is working!";

	char *var = calloc(1, 100);

	strcpy(var, test_string);

	ASSERT_TRUE(
	        "27.allocated calloc memory should contain the copied value",
	        strcmp(var, test_string) == 0);

	free(var);
}

static void
calloc_with_zero_size_returns_null_pointer(void)
{
	size_t var = (size_t) calloc(0, 100);

	ASSERT_TRUE("28. calloc with zero size returns non null pointer",
	            var == NULL);
}

static void
calloc_with_zero_nmembs_returns_null_pointer(void)
{
	size_t var = (size_t) calloc(100, 0);

	ASSERT_TRUE("29. calloc with zero nmemb returns non null pointer",
	            var == NULL);
}

static void
calloc_of_size_bigger_than_large_returns_null_pointer(void)
{
	size_t var = (size_t) calloc(1, LARGE + 1);

	ASSERT_TRUE("30. calloc of size bigger than large returns null pointer",
	            var == NULL);
}

static void
malloc_of_size_bigger_than_large_returns_null_pointer(void)
{
	size_t var = (size_t) malloc(LARGE + 1);

	ASSERT_TRUE("31. malloc of size bigger than large returns null pointer",
	            var == NULL);
}

static void
realloc_with_null_pointer_returns_non_null_pointer(void)
{
	size_t var = (size_t) realloc(NULL, 100);

	ASSERT_TRUE("32. realloc with null pointer returns non null pointer",
	            var != NULL);
}

static void
realloc_with_size_zero_works_as_a_free(void)
{
	void *var = malloc(100);
	void *var2 = malloc(100);
	void *var3 = realloc(var, 0);
	struct region *first_region =
	        (struct region *) ((size_t) var - sizeof(struct region));

	ASSERT_TRUE("33. realloc with size zero works as a free",
	            first_region->free == true);
}

static void
freeing_the_only_region_of_a_block_frees_the_block(void)
{
	struct malloc_stats stats;
	size_t var = (size_t) malloc(100);

	free(var);

	get_stats(&stats);

	ASSERT_TRUE("34. freeing the only region of a block frees the block",
	            stats.blocks_counter == 0);
}

static void
double_freeing_the_same_region_does_not_change_stats(void)
{
	struct malloc_stats stats;
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);

	free(var);
	free(var);

	ASSERT_TRUE("35. double freeing the same region sets errno to ENOMEM",
	            errno == ENOMEM);
}

static void
realloc_to_a_smaller_size_use_the_same_region(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);

	size_t var3 = (size_t) realloc(var, 50);

	ASSERT_TRUE("36. realloc to a smaller size use the same region",
	            var3 == var);
}

static void
realloc_of_a_freed_region_returns_null_pointer(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);

	free(var);

	size_t var3 = (size_t) realloc(var, 50);

	ASSERT_TRUE("37. realloc of a freed region returns null pointer",
	            var3 == NULL);
}

static void
realloc_to_a_bigger_region_of_a_region_with_no_next_allocs_to_new_region(void)
{
	size_t var = (size_t) malloc(2008);
	size_t var3 = (size_t) realloc(var, 2048);

	ASSERT_TRUE("38. realloc to a bigger region of a region with no next "
	            "allocs "
	            "to new region",
	            var3 != var);
}

static void
realloc_to_a_bigger_region_with_next_region_not_freed_allocs_to_new_region(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);
	size_t var3 = (size_t) realloc(var, 200);

	ASSERT_TRUE("39. realloc to a bigger region with next region not freed "
	            "allocs to new region",
	            var3 != var);
}

static void
realloc_if_next_region_is_free_and_big_enough_use_it(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);
	size_t var3 = (size_t) malloc(100);

	free(var2);

	size_t var4 = (size_t) realloc(var, 200);

	ASSERT_TRUE("40. realloc if next region is free and big enough use it",
	            var4 == var);
}

static void
realloc_if_next_region_is_free_but_not_big_enough_dont_use_it(void)
{
	size_t var = (size_t) malloc(100);
	size_t var2 = (size_t) malloc(100);
	size_t var3 = (size_t) malloc(100);

	free(var2);

	size_t var4 = (size_t) realloc(var, 300);

	ASSERT_TRUE(
	        "41. realloc if next region is free but not big enough dont "
	        "use it",
	        var4 != var);
}

int
main(void)
{
	run_test(successful_malloc_returns_non_null_pointer);
	run_test(correct_copied_value);
	run_test(correct_amount_of_mallocs);
	run_test(correct_amount_of_frees);
	run_test(correct_amount_of_requested_memory);
	run_test(correct_copied_values);
	run_test(malloc_bigger_than_block);
	run_test(not_first_malloc_bigger_than_block);
	run_test(sum_of_malloc_sizes_bigger_than_block);
	run_test(ptr_to_malloc_returns_correct_position);
	run_test(use_freed_region);
	run_test(coalesing_with_previous_region);
	run_test(coalesing_with_previous_region_also_includes_region_header);
	run_test(cant_use_freed_region_if_malloc_size_is_bigger);
	run_test(coalesing_with_following_region);
	run_test(coalesing_with_following_region_also_includes_region_header);
	run_test(malloc_of_a_small_enough_space_allows_splitting);
	run_test(malloc_of_size_bigger_than_small_creates_a_bigger_region);
	run_test(freeing_all_regions_of_last_block_frees_block);
	run_test(freeing_all_regions_of_middle_block_frees_block);
	run_test(testing_finding_free_regions);
	run_test(calloc_returns_non_null_pointer);
	run_test(calloc_initializes_memory_to_zero);
	run_test(calloc_correct_copied_value);
	run_test(calloc_with_zero_size_returns_null_pointer);
	run_test(calloc_with_zero_nmembs_returns_null_pointer);
	run_test(calloc_of_size_bigger_than_large_returns_null_pointer);
	run_test(malloc_of_size_bigger_than_large_returns_null_pointer);
	run_test(realloc_with_null_pointer_returns_non_null_pointer);
	run_test(realloc_with_size_zero_works_as_a_free);
	run_test(freeing_the_only_region_of_a_block_frees_the_block);
	run_test(double_freeing_the_same_region_does_not_change_stats);
	run_test(realloc_to_a_smaller_size_use_the_same_region);
	run_test(realloc_of_a_freed_region_returns_null_pointer);
	run_test(realloc_to_a_bigger_region_of_a_region_with_no_next_allocs_to_new_region);
	run_test(realloc_to_a_bigger_region_with_next_region_not_freed_allocs_to_new_region);
	run_test(realloc_if_next_region_is_free_and_big_enough_use_it);
	run_test(realloc_if_next_region_is_free_but_not_big_enough_dont_use_it);
	return 0;
}