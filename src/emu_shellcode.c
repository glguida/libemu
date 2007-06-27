
#include "emu/emu_shellcode.h"

#include "emu/emu.h"
#include "emu/emu_memory.h"
#include "emu/emu_cpu.h"
#include "emu/emu_cpu_data.h"
#include "emu/emu_track.h"
#include "emu/emu_source.h"
#include "emu/emu_getpc.h"
#include "emu/environment/win32/emu_env_w32.h"
#include "emu/environment/win32/emu_env_w32_dll_export.h"
#include "emu/emu_hashtable.h"
#include "emu/emu_graph.h"
#include "emu/emu_list.h"
#include "emu/emu_queue.h"

#include "emu/emu_log.h"


#define STATIC_OFFSET 0x00471000
#define EMU_SHELLCODE_TEST_MAX_STEPS 128


bool cmp_uint32_t(void *a, void *b)
{
	if ( (uint32_t)a == (uint32_t)b )
		return true;

	return false;
}

uint32_t hash_uint32_t(void *key)
{
	uint32_t ukey = (uint32_t)key;
	ukey++;
	return ukey;
}


int32_t     emu_shellcode_run_and_track(struct emu *e, 
										uint8_t *data, 
										uint16_t datasize, 
										uint16_t eipoffset,
										uint32_t steps,
										struct emu_env_w32 *env, 
										struct emu_track_and_source *etas,
										struct emu_hashtable *known_positions,
										struct emu_stats *running_stats
									   )
{
	struct emu_cpu *cpu = emu_cpu_get(e);
	struct emu_memory *mem = emu_memory_get(e);


	struct emu_queue *eq = emu_queue_new();
	emu_queue_enqueue(eq, (void *)((uint32_t)eipoffset+STATIC_OFFSET));

	while ( !emu_queue_empty(eq) )
	{

		uint32_t current_offset = (uint32_t)emu_queue_dequeue(eq);

		/* init the cpu/memory 
		 * scooped to keep number of used varnames small 
		 */
        {
			logDebug(e, "running at offset %i %08x\n", current_offset, current_offset);

			/* write the code to the offset */
			emu_memory_write_block(mem, STATIC_OFFSET, data, datasize);

			/* set the registers to the initial values */
			int reg;
			for ( reg=0;reg<8;reg++ )
				emu_cpu_reg32_set(cpu,reg ,0x0);

			emu_cpu_reg32_set(cpu, esp, 0x00120000);
			emu_cpu_eip_set(cpu, current_offset);

			/* set the flags */
			emu_cpu_eflags_set(cpu,0x0);
		}

		emu_tracking_info_clear(&etas->track);

		int j;
		for ( j=0;j<steps;j++ )
		{
			uint32_t eipsave = emu_cpu_eip_get(cpu);

			struct emu_env_w32_dll_export *dllhook = NULL;

			dllhook = emu_env_w32_eip_check(env);


			if ( dllhook != NULL )
			{
				if ( dllhook->fnhook == NULL )
					break;
			}
			else
			{

				int32_t ret = emu_cpu_parse(emu_cpu_get(e));
				if ( ret == -1 )
					break;

				ret = emu_cpu_step(emu_cpu_get(e));
				if ( ret == -1 )
					break;

				if ( emu_track_instruction_check(e, etas) == -1 )
				{
					logDebug(e, "failed instr %s\n", cpu->instr_string);
					logDebug(e, "tracking at eip %08x\n", eipsave);
					if ( cpu->instr.is_fpu )
					{

					}
					else
					{

						/* save the requirements of the failed instruction */
						struct emu_tracking_info *instruction_needs_ti = emu_tracking_info_new();
						emu_tracking_info_copy(&cpu->instr.cpu.track.need, instruction_needs_ti);

						struct emu_queue *bfs_queue = emu_queue_new();

						/*
						 * the current starting point is the first position used to bfs
						 * scooped to avoid varname collisions 
						 */
						{ 
							struct emu_tracking_info *eti = emu_tracking_info_new();
							emu_tracking_info_diff(&cpu->instr.cpu.track.need, &etas->track, eti);
							eti->eip = current_offset;
							emu_tracking_info_debug_print(eti);
							emu_queue_enqueue(bfs_queue, eti);
						}

						while ( !emu_queue_empty(bfs_queue) )
						{
							logDebug(e, "loop %s:%i\n", __FILE__, __LINE__);

							struct emu_tracking_info *current_pos_ti_diff = (struct emu_tracking_info *)emu_queue_dequeue(bfs_queue);
							struct emu_hashtable_item *current_pos_ht = emu_hashtable_search(etas->static_instr_table, (void *)current_pos_ti_diff->eip);
							if (current_pos_ht == NULL)
							{
								logDebug(e, "current_pos_ht is NULL?\n");
								exit(-1);
							}

							struct emu_vertex *current_pos_v = (struct emu_vertex *)current_pos_ht->value;
							struct emu_source_and_track_instr_info *current_pos_satii = (struct emu_source_and_track_instr_info *)current_pos_v->data;

							if (current_pos_v->color == red)
								continue;

							current_pos_v->color = red;

							while ( !emu_tracking_info_covers(&current_pos_satii->track.init, current_pos_ti_diff) )
							{
								logDebug(e, "loop %s:%i\n", __FILE__, __LINE__);
								if ( current_pos_v->backlinks == 0 )
								{
									break;
								}
								else
								if ( current_pos_v->backlinks > 1 )
								{ /* queue all to diffs to the bfs queue */
									struct emu_edge *ee;
									struct emu_vertex *ev;
									for ( ee = emu_edges_first(current_pos_v->backedges); !emu_edges_attail(ee); ee=emu_edges_next(ee) )
									{
										ev = ee->destination;
										struct emu_tracking_info *eti = emu_tracking_info_new();
										emu_tracking_info_diff(&current_pos_satii->track.init, current_pos_ti_diff, eti);
										eti->eip = ((struct emu_source_and_track_instr_info *)ev->data)->eip;
										emu_queue_enqueue(bfs_queue, eti);

									}
									/* the new possible positions and requirements got queued into the bfs queue, 
									 *  we break here, so the new queued positions can try to work it out
									 */
									break;
								}
								else
								if ( current_pos_v->backlinks == 1 )
								{ /* follow the single link */
									current_pos_v = emu_edges_first(current_pos_v->backedges)->destination;
									current_pos_satii = (struct emu_source_and_track_instr_info *)current_pos_v->data;
									emu_tracking_info_diff(current_pos_ti_diff, &current_pos_satii->track.init, current_pos_ti_diff);
								}
							}

							if ( emu_tracking_info_covers(&current_pos_satii->track.init, current_pos_ti_diff) )
							{
								logDebug(e, "found position which satiesfies the requirements %i %08x\n", current_pos_satii->eip, current_pos_satii->eip);
								emu_tracking_info_debug_print(&current_pos_satii->track.init);
								emu_queue_enqueue(eq, (void *)((uint32_t)current_pos_satii->eip));
							}
						}
					}
					/* the shellcode did not run correctly as he was missing instructions initializing required registers
					 * we did what we could do in the prev lines of code to find better offsets to start from
					 * the working offsets got queued into the main queue, so we break here to give them a chance
                     */
					break;
				}else
				{
					logDebug(e, "%s\n", cpu->instr_string);
				}
			}
		}
		return j;
	}

	return -1;
}


enum
{
	EMU_SCTEST_SUSPECT_GETPC,
	EMU_SCTEST_SUSPECT_MOVFS
} emu_shellcode_suspect;



int32_t emu_shellcode_test(struct emu *e, uint8_t *data, uint16_t size)
{
	logPF(e);
/*

	
	bool found_good_candidate_after_getpc = false;

	uint32_t best_eip=0;
*/
	uint32_t offset;
	struct emu_list_root *el;
	el = emu_list_create();


	for ( offset=0; offset<size ; offset++ )
	{
		if ( emu_getpc_check(e, (uint8_t *)data, size, offset) != 0 )
		{
			logDebug(e, "possible getpc at offset %i (%08x)\n", offset, offset);
			struct emu_list_item *eli = malloc(sizeof(struct emu_list_item));
			memset(eli, 0, sizeof(struct emu_list_item));
			emu_list_init_link(eli);
			eli->uint32 = offset;
			emu_list_insert_last(el, eli);
		}
	}

	if ( emu_list_length(el) == 0 )
	{
		emu_list_destroy(el);
		return -1;
	}

	{
		struct emu_cpu *cpu = emu_cpu_get(e);
		struct emu_memory *mem = emu_memory_get(e);

		/* write the code to the offset */
		emu_memory_write_block(mem, STATIC_OFFSET, data, size);

		/* set the registers to the initial values */
		int reg;
		for ( reg=0;reg<8;reg++ )
			emu_cpu_reg32_set(cpu,reg ,0x0);

		emu_cpu_reg32_set(cpu, esp, 0x00120000);
		emu_cpu_eip_set(cpu, 0);

		/* set the flags */
		emu_cpu_eflags_set(cpu,0x0);
	}

	struct emu_track_and_source *etas = emu_track_and_source_new();

	logDebug(e, "creating static callgraph\n");
	/* create the static analysis graph 
	set memory read only so instructions can't change it*/
	emu_memory_mode_ro(emu_memory_get(e));
	emu_source_instruction_graph_create(e, etas, STATIC_OFFSET, size);
	emu_memory_mode_rw(emu_memory_get(e));

	struct emu_hashtable *eh = emu_hashtable_new(size+4/4, hash_uint32_t, cmp_uint32_t);
	struct emu_list_item *eli;
	struct emu_env_w32 *env = emu_env_w32_new(e);
	struct emu_stats *stats = NULL;

	int steps = -1;
	for ( eli = emu_list_first(el); !emu_list_attail(eli); eli = emu_list_next(eli) )
	{
		logDebug(e, "testing offset %i %08x\n", eli->uint32, eli->uint32);
/*		emu_shellcode_run_and_track(struct emu *e, 
									 uint8_t *data, 
									 uint16_t datasize, 
									 uint16_t eipoffset,
									 uint32_t steps,
									 struct emu_env_w32 *env, 
									 struct emu_track_and_source *etas,
									 struct emu_hashtable *eh,
									 struct emu_stats *est
									 );
*/


		steps = emu_shellcode_run_and_track(e, data, size, eli->uint32, 256, env, etas, eh, stats);
	}

	emu_hashtable_free(eh);
	emu_list_destroy(el);
	return steps;
}