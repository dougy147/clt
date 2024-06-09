#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

/*
   Concepts Learning Task constructed following this paper (minor differences at the moment):
   Mathy, F., & Bradmetz, J. (2003). A theory of the graceful complexification of concepts and their learnability. Current Psychology of Cognition, 22(1), 41-82.
*/

#define D 3 // Dimensions = number of conjunction of features (not configurable yet)
#define DEBUG 0

enum {
    C_RED,  // red (avoids conflict with raylib's RED color)
    C_BLUE, // blue
    NB_COLORS,
}; // stimuli colors

enum {
    SQUARE,
    CIRCLE,
    NB_SHAPES,
}; // stimuli shape

enum {
    SMALL,
    BIG,
}; // stimuli size

typedef struct stimulus {
    int shape;
    int size;
    int color;
} Stimulus;

typedef struct concept {
    int pe;            // Number of positive examples in the concept
    int eq;            // Number of equivalent subsets for this concept (used to check concordance)
    int label ;        // This program's labeling number (depends on how concepts are constructed)
    int authors_label; // Original label by the authors
    Stimulus positive_examples[1<<4];
} Concept;

int factorial(int n) {
  int f = 1;
  while (n > 0) { f *= n; n--; }
  return f;
}

int C(int k, int n) {
  /*Combinations of k elem from a set of n elem*/
  return factorial(n) / (factorial(k) * factorial(n-k)) ;
}

Stimulus generate_3d_stimulus(int shape, int size, int color) {
    Stimulus stimulus = { .shape=shape, .size=size, .color=color, };
    return stimulus;
}

void populate_examples(Stimulus examples_array[], int acc[], int index_acc) {
    /*Generate all examples given the number of dimensions (D), and populate
    the examples' array. In 3D, this gives: 000,001,010,011,100,101,110,111*/
    static int subsets_counter = 0;
    if ( index_acc == D ) {
      Stimulus stimulus;
      stimulus = generate_3d_stimulus(acc[0], acc[1], acc[2]);
      examples_array[subsets_counter] = stimulus;
      subsets_counter++;
      return;
    }
    int acc_0[D], acc_1[D];
    for (int i = 0; i < D; i++) {
	acc_0[i] = acc[i];
	acc_1[i] = acc[i];
    }
    acc_0[index_acc] = 0;
    acc_1[index_acc] = 1;
    populate_examples(examples_array, acc_0, index_acc+1);
    populate_examples(examples_array, acc_1, index_acc+1);
}

void populate_subsets(Stimulus stimuli[], int k, Stimulus subsets[][k]) {
    /*Construct all subsets of k element (positive examples) in dimension D */
    int n = 1 << (1<<D);
    int l2 = log2(n); // base 2 logarithm of n
    int one_counter;
    int subsets_counter = 0;
    for (int i = 1; i < n; i++) {
	one_counter = 0;
	int ibin = i; // binary representation of i
	int indexes_array[1<<D] = {-1}; // store indexes where an example can be placed
	for (int j = 0; j < l2; j++) {
	    if (ibin & 1) {
		indexes_array[one_counter] = j;
		one_counter++;
		if (one_counter > k) {
		    one_counter = 0;
		    break;
		}
	    }
	    ibin = ibin >> 1;
	}
	if (one_counter == k) {
	    for (int x = 0; x < k; x++) {
		subsets[subsets_counter][x] = stimuli[indexes_array[x]];
	    }
	    subsets_counter++;
	}
    }
}

void ascending_sort(int *array, size_t size) { // dumb but works
    int value_ordered = 0;
    while (value_ordered < size) {
	int min_value = 99999;
	int min_index = value_ordered;
	for (int i=value_ordered; i < size; i++) {
	    if (array[i] < min_value) {
		min_value = array[i];
		min_index = i;
	    }
    	}
	int swap_val = array[value_ordered];
	array[value_ordered] = min_value;
	array[min_index] = swap_val;
	value_ordered++;
    }
}

void ascending_sort_concepts(Concept *array, size_t size) { // dumb but works
    /*TODO: refacto with above function (how to any type for a function?)*/
    int value_ordered = 0;
    while (value_ordered < size) {
	Concept min_concept;
	int min_value = 99999;
	int min_index = value_ordered;
	for (int i=value_ordered; i < size; i++) {
	    if (array[i].authors_label < min_value) {
		min_concept = array[i];
		min_value = array[i].authors_label;
		min_index = i;
	    }
    	}
	Concept swap_val = array[value_ordered];
	array[value_ordered] = min_concept;
	array[min_index] = swap_val;
	value_ordered++;
    }
}

bool compute_3d_equivalence(Stimulus *subset1, Stimulus *subset2, size_t size) {
    /*Given two subsets, check if they are logically equivalent*/
    int distances_subset1[(size*(size-1))/2]; // store each computed distances (subset 1)
    int distances_subset2[(size*(size-1))/2]; // store each computed distances (subset 2)
    int index = 0;
    for (int i = 0; i < size - 1; i++) {
	for (int j = i+1; j < size; j++) {
	    /*XOR features to compute distance*/
	    int d1 = (subset1[i].shape ^ subset1[j].shape) +
		(subset1[i].size ^ subset1[j].size) +
		(subset1[i].color ^ subset1[j].color); // distance for a pair of examples in subset1
	    int d2 = (subset2[i].shape  ^ subset2[j].shape) +
		(subset2[i].size  ^ subset2[j].size) +
		(subset2[i].color ^ subset2[j].color); // distance for a pair of examples in subset2
	    distances_subset1[index] = d1;
	    distances_subset2[index] = d2;
	    index++;
	}
    }
    ascending_sort(distances_subset1, size*(size-1) / 2);
    ascending_sort(distances_subset2, size*(size-1) / 2);
    for (int i = 0; i < size*(size-1) / 2; i++) {
	if (distances_subset1[i] != distances_subset2[i]) {
	    return false;
	}
    }
    return true;
}

int count_concepts(Stimulus *examples) {
    int unique_concepts = 0;
    for (int k = 1; k < (1<<D) / 2 + 1; k++) {
	int nb_subsets = C(k, 1<<D);
	int categorized_subsets[nb_subsets];

	Stimulus subsets_array[nb_subsets][k];
	populate_subsets(examples, k , subsets_array);

	/*Count concepts of k elements. */
	int index_categorized_subsets = 0;
	for (int i = 0; i < C(k,1<<D) - 1; i++) {
	    bool already_categorized_i = false;
	    for (int j = i+1; j < C(k,1<<D); j++) {
		bool already_categorized_j = false;
		for (int x = 0; x < index_categorized_subsets; x++) {
		    if (categorized_subsets[x] == j) { already_categorized_j = true;}
		}
		if (already_categorized_j) { continue;}
		if (compute_3d_equivalence(subsets_array[i], subsets_array[j], k)) {
		    if (!already_categorized_i) {
			categorized_subsets[index_categorized_subsets] = i;
			index_categorized_subsets++;
			already_categorized_i = true;
			unique_concepts++;
		    }
		    categorized_subsets[index_categorized_subsets] = j;
		    index_categorized_subsets++;
		}
	    }
	}
    }
    return unique_concepts;
}

void randomly_populate_concepts(Concept concepts[], int concept_to_generate, Stimulus *examples) {
    /*TODO: refacto with above function*/
    int j_indexes[1000];
    int j_index = 0;
    bool matched = false;

    int unique_concept = 0;
    for (int k = 1; k < (1<<D) / 2 + 1; k++) {
	int nb_subsets = C(k, 1<<D);
	int categorized_subsets[nb_subsets];

	Stimulus subsets_array[nb_subsets][k];
	populate_subsets(examples, k , subsets_array);

	/*Count concepts of k elements. */
	int index_categorized_subsets = 0;
	for (int i = 0; i < C(k,1<<D) - 1; i++) {
	    bool already_categorized_i = false;
	    for (int j = i+1; j < C(k,1<<D); j++) {
		bool already_categorized_j = false;
		for (int x = 0; x < index_categorized_subsets; x++) {
		    if (categorized_subsets[x] == j) { already_categorized_j = true;}
		}
		if (already_categorized_j) { continue;}
		if (compute_3d_equivalence(subsets_array[i], subsets_array[j], k)) {
		    if (!already_categorized_i) {
			categorized_subsets[index_categorized_subsets] = i;
			index_categorized_subsets++;
			already_categorized_i = true;
			unique_concept++;
			if (matched == false && unique_concept == concept_to_generate) {
			    matched = true;
			    j_indexes[j_index] = i;
			    j_index++;
			}
		    }
		    categorized_subsets[index_categorized_subsets] = j;
		    index_categorized_subsets++;
		    if (unique_concept == concept_to_generate) {
			j_indexes[j_index] = j;
			j_index++;
		    }
		}
	    }
	}
	if (matched) {
	    int random_concept = rand() % j_index;
	    Concept c;
	    c.pe = k;
	    c.eq = j_index;
	    c.label = concept_to_generate;
	    for (int i = 0; i < k; i++) {
		c.positive_examples[i] = subsets_array[j_indexes[random_concept]][i];
	        concepts[concept_to_generate-1] = c;
	    }
	    return;
	}
    }
}

bool is_stimulus_positive_example(Stimulus stimulus, Concept concept) {
    for (size_t i = 0; i < concept.pe; i++) {
	//if (stimulus == positive_examples[i]) return true; // outch, not really possible in C
	if (stimulus.shape  != concept.positive_examples[i].shape)  continue;
	if (stimulus.size  != concept.positive_examples[i].size)  continue;
	if (stimulus.color != concept.positive_examples[i].color) continue;
	return true;
    }
    return false;
}

void draw_stimulus(Stimulus stimulus, int w, int h) {
    int small_size = w/6;
    int big_size   = w/3;
    if (stimulus.shape == SQUARE) {
	int side = stimulus.size == SMALL ? small_size : big_size;
	DrawRectangle(w/2 - side/2, h/2 - side/2, side, side, stimulus.color == C_RED ? RED : BLUE);
    } else if (stimulus.shape == CIRCLE) {
	int side = stimulus.size == SMALL ? small_size : big_size;
	DrawCircle(w/2, h/2, side/2, stimulus.color == C_RED ? RED : BLUE);
    }
}

void draw_progress_bar(int streak, int window_width, int window_height) {
    int size_streak = (1<<D)*2;
    int block_size  = 0.02 * window_width ; // size of a square
    int blocks_padding = 0.1 * block_size;
    int wo = (window_width / 2) - (size_streak * 0.5 * block_size) - ((size_streak / 2 - 1) * blocks_padding) ; // width offset
    int ho = 0.85 * window_height;
    for (int i = 0; i < size_streak; i++) {
	DrawRectangle(
		wo + i*(block_size + blocks_padding),
		ho,
		block_size,
		block_size,
		i+1 <= streak ? GREEN : DARKGRAY
		);
    }
}

void label_from_paper(Concept *concept) {
    /*3D concepts from this program are not labeled like in the paper.
      This function attributes correct labels given ordered distances.*/
    int authors_concepts_sizes[13]   = {4,2,1,3,2,4,4,3,2,4,4,3,4};
    Stimulus authors_concepts[13][4] =
	{{{1,1,1}, {1,1,0}, {0,1,0}, {0,1,1}},
	{{1,1,1}, {1,1,0}},
    	{{1,1,1}},
    	{{1,1,1}, {1,1,0}, {0,1,0}},
    	{{1,1,1}, {0,1,0}},
    	{{1,1,1}, {1,1,0}, {0,0,0}, {0,0,1}},
    	{{1,1,1}, {1,1,0}, {0,1,0}, {0,0,0}},
    	{{1,1,1}, {1,1,0}, {0,0,1}},
    	{{1,1,1}, {0,0,0}},
    	{{1,1,1}, {1,1,0}, {0,1,0}, {1,0,0}},
    	{{1,1,1}, {1,1,0}, {0,1,0}, {0,0,1}},
    	{{1,1,1}, {1,0,0}, {0,1,0}},
    	{{1,1,1}, {1,0,0}, {0,1,0}, {0,0,1}}};

    for (int i = 0; i < 13; i++) {
	if (authors_concepts_sizes[i] != concept->pe) continue;
	if (compute_3d_equivalence(authors_concepts[i], concept->positive_examples, concept->pe)) {
	    concept->authors_label = i+1;
	    return;
	}
    }
}

int main(void) {
    srand(time(NULL));
    /*
       In n-dimensions, we'll have 2**n of examples.
       In 3D, this gives the cubes below (examples/label):

		   111 			1
	    110           011	    2       4
	           010                  6
		   101                  3
	    100           001       5       7
	           000                  8
    */
    Stimulus examples[1<<D]; // shifting 1 x times = 2 ** x
    int accumulator[D];
    populate_examples(examples, accumulator, 0);

    /*Task needed parameters*/
    int nb_concepts = count_concepts(examples); // nb of NON EQUIVALENT concepts (13 in 3D)

    /* Randomly select one subset for each concept (among all their equivalence) (from 1 to 13 in 3D)*/
    Concept concepts[nb_concepts];
    for (int i = 0; i < nb_concepts; i++) {
	randomly_populate_concepts(concepts, i+1, examples);
	/*Labelize each concept with original paper label*/
	label_from_paper(&concepts[i]);
    }
    ascending_sort_concepts(concepts, nb_concepts); // Order concepts from less to most complex.

    int concepts_index = 0;
    int stimuli_index = 0;
    int streak = 0; // 16 successive correct answers switch to new concept
    int concepts_learned = 0;
    int end_game = false;

    Stimulus current_stimulus = examples[stimuli_index];
    Concept current_concept = concepts[concepts_index];

    /*Window parameters*/
    int factor = 50;
    int w = 16 * factor;
    int h = 16 * factor;
    InitWindow(w,h,"Concept learning");
    SetTargetFPS(30);

    while (!WindowShouldClose()) {
	BeginDrawing();
	ClearBackground(BLACK);

	if (!end_game) {
	    /*Indicate current "level"*/
	    DrawText(TextFormat("Level %d", concepts[concepts_index].authors_label),  w/2 - (20 * 4),  0.08 * h,  40, RED);

	    draw_stimulus(examples[stimuli_index], w,h);

	    /*DEBUG*/
	    #if DEBUG
	    DrawText(TextFormat("N° %d", concepts[concepts_index].authors_label),  w/2 - (20 * 4),  0.16 * h,  40, RED);
	    DrawText(TextFormat("Concept %d (%d ex+; from %d equivalent sets) - complexity TODO",
	    	    concepts[concepts_index].label,
	    	    concepts[concepts_index].pe,
	    	    concepts[concepts_index].eq
	    	    ),  0,  0,  20, RED);
	    /*Draw help (debug only :p)*/
	    if (is_stimulus_positive_example(current_stimulus, current_concept)) {
	        DrawText("Positive example!",  0,  h-40,  20, GREEN);
	    }
	    #endif


	    /*For the moment LEFT CLICK == YES (belongs to concept) ; RIGHT CLICK == NO*/
	    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && is_stimulus_positive_example(current_stimulus,current_concept) ||
		    IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && !is_stimulus_positive_example(current_stimulus,current_concept)) {
		    streak++;
		} else {
		    streak = 0; // ERROR: bad categorization
		}
	        /*Rotate stimuli*/
	        stimuli_index++; // TODO: choose random index (different from current)
	        if (stimuli_index >= 1<<D) {
	            stimuli_index=0;
	        }
	        current_stimulus = examples[stimuli_index];

	        /*Rotate concept if needed*/
		if (streak == (1<<D) * 2) {
		    streak = 0;
		    concepts_learned++;
		    concepts_index++;
		    current_concept = concepts[concepts_index];
		}
	    }

	    /*Display streak in progress bar*/
	    draw_progress_bar(streak, w, h);

	    /*End game*/
	    if (concepts_learned == nb_concepts) {
	        end_game = true;
	    }
	} else { // if end game = true
	    DrawText("Congratulations!\n\nYou beat the game!",w/2-20*6, h/2,40,YELLOW);
	}

	EndDrawing();
    }
    return 0;
}
