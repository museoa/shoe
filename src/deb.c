/* deb.c
 *
 * COPYRIGHT (c) 1998 by Fredrik Noring.
 *
 * This is the debug module.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME  "debug"

#include "types.h"

#include "args.h"
#include "big.h"
#include "bif.h"
#include "map.h"
#include "str.h"
#include "vec.h"
#include "pair.h"
#include "process.h"

BIF_DECLARE(bif_debug_objects)
{
  struct map *map;
  INT i;

  ARGS_GET((process, "debug-objects", args, ""));

  map = map_allocate(&process->map_heap);
  
  BIF_RESULT_MAPPING(map);   /* Very important for the garb! */

#ifdef USE_BIG_INTEGERS
  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "big-integers",
				  big_debug_objects(&process->big_heap));
#endif /* USE_BIG_INTEGERS */
  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "strings",
				  str_debug_objects(&process->str_heap));
  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "mappings",
				  map_debug_objects(&process->map_heap));
  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "vectors",
				  vec_debug_objects(&process->vec_heap));
  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "pairs",
				  pair_debug_objects(&process->pair_heap));
}

BIF_DECLARE(bif_debug_memory)
{
  struct map *map;
  INT i;

  ARGS_GET((process, "debug-memory", args, ""));

  map = map_allocate(&process->map_heap);
  
  BIF_RESULT_MAPPING(map);   /* Very important for the garb! */

#ifdef USE_BIG_INTEGERS
  BIF_MAP_TEXT_WITH_UNDEFINED(map, "big-integers");
#endif /* USE_BIG_INTEGERS */
  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "strings",
				  str_debug_memory(&process->str_heap));
  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "mappings",
				  map_debug_memory(&process->map_heap));
  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "vectors",
				  vec_debug_memory(&process->vec_heap));
  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "pairs",
				  pair_debug_memory(&process->pair_heap));
}

BIF_DECLARE(bif_debug_pairs)
{
  struct map *map;
  INT i;

  ARGS_GET((process, "debug-pairs", args, ""));

  map = map_allocate(&process->map_heap);
  
  BIF_RESULT_MAPPING(map);   /* Very important for the garb! */

  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "objects",
				  pair_debug_objects(&process->pair_heap));
  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "objects-used",
				 pair_debug_objects_used(&process->pair_heap));
  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "memory",
				  pair_debug_memory(&process->pair_heap));
  BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, "memory-used",
				  pair_debug_memory_used(&process->pair_heap));
}
