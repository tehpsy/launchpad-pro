/******************************************************************************
 
 Copyright (c) 2015, Focusrite Audio Engineering Ltd.
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 * Neither the name of Focusrite Audio Engineering Ltd., nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 *****************************************************************************/

//______________________________________________________________________________
//
// Headers
//______________________________________________________________________________

#include "app.h"
#include <stdbool.h>
#include <stdlib.h>

//______________________________________________________________________________
//
// Defines
//______________________________________________________________________________

#define NUM_ROWS 8
#define NUM_COLUMNS 8
#define GRABBER_TICKS 50
#define COMPLETION_TICKS 1000

typedef enum GameState {
	GameState_INIT = 0,
	GameState_RUNNING,
	GameState_GAME_COMPLETE,
} GameState;

typedef enum GrabType {
	GrabType_TOP = 0,
	GrabType_BOTTOM,
	GrabType_LEFT,
	GrabType_RIGHT,
	
	GrabType_NBR_ELT,
	GrabType_NONE
} GrabType;

typedef enum ProgressResult {
	ProgressResult_OK = 0,
	ProgressResult_BLOCKED,
	ProgressResult_COMPLETE
} ProgressResult;

typedef enum CellType {
	CellType_EMPTY = 0,
	CellType_NUGGET,
	
	CellType_GRABBER_TOP,
	CellType_GRABBER_BOTTOM,
	CellType_GRABBER_LEFT,
	CellType_GRABBER_RIGHT,
	
	CellType_NBR_ELT
} CellType;

typedef enum GrabberState {
	GrabberState_STOPPED = 0,
	GrabberState_EXTENDING,
	GrabberState_RETRACTING,
	GrabberState_RETRACTING_RESTART,
	
	GrabberState_NBR_ELT
} GrabberState;

struct Grabber {
	int index;
	int next_index;
	GrabberState state;
	int length;
	GrabType type;
	int tick;
};

struct Color {
	u8 r;
	u8 g;
	u8 b;
};

void score(GrabType grabType);
void game_complete(GrabType grabType);
void reset_grabbers();
void reset_cells();
void update_cells();
void setup_nugget();
void update_LEDs();
void reset_game();
ProgressResult can_progress(GrabType type);
void update_score_LEDs();

struct Color colors[4] = {
	{MAXLED, 0, 0},
	{0, MAXLED, 0},
	{0, 0, MAXLED},
	{MAXLED, MAXLED, 0}
};
u8 nugget_row = 0;
u8 nugget_col = 0;
u8 scores[4] = {0, 0, 0, 0};
GameState game_state = GameState_INIT;
CellType cells[NUM_ROWS][NUM_COLUMNS];
struct Grabber grabbers[4];

//______________________________________________________________________________
//
// Setup
//______________________________________________________________________________

void app_init()
{
	reset_game();
	setup_nugget();
}

void setup_nugget()
{
	u8 row = 0;
	u8 col = 0;
	
	bool matched = false;
	
	while (!matched)
	{
		int randRow = rand() % NUM_ROWS;
		int randCol = rand() % NUM_COLUMNS;
		
		CellType cell = cells[randRow][randCol];
		matched = (cell == CellType_EMPTY);
		
		if (matched)
		{
			row = randRow;
			col = randCol;
		}
	}
	
	cells[row][col] = CellType_NUGGET;
	nugget_col = col;
	nugget_row = row;
}


//______________________________________________________________________________
//
// Input
//______________________________________________________________________________

void handle_input(u8 type, u8 padIndex, u8 value)
{
	GrabType grabType = GrabType_NONE;
	u8 grabIndex = 0;
	
	switch (type)
	{
		case TYPEPAD:
		{
			if (value != 0)
			{
				if (padIndex % 10 == 0)
				{
					grabIndex = (padIndex/10) - 1;
					grabType = GrabType_LEFT;
				}
				else if (padIndex % 10 == 9)
				{
					grabIndex = (padIndex/10) - 1;
					grabType = GrabType_RIGHT;
				}
				else if (padIndex > 90 && padIndex < 99)
				{
					grabIndex = (padIndex % 10) - 1;
					grabType = GrabType_TOP;
				}
				else if (padIndex < 10)
				{
					grabIndex = (padIndex % 10) - 1;
					grabType = GrabType_BOTTOM;
				}
				
				if ((grabType < GrabType_NBR_ELT) && (grabIndex < 8))
				{
					if (grabbers[grabType].state == GrabberState_STOPPED)
					{
						//start grabber
						grabbers[grabType].index = grabIndex;
						grabbers[grabType].state = GrabberState_EXTENDING;
						grabbers[grabType].tick = GRABBER_TICKS;
					}
					else if (grabbers[grabType].state == GrabberState_EXTENDING)
					{
						if (grabbers[grabType].index == grabIndex)
						{
							//nothing
						}
						else
						{
							grabbers[grabType].state = GrabberState_RETRACTING_RESTART;
							grabbers[grabType].next_index = grabIndex;
						}
					}
					else if (grabbers[grabType].state == GrabberState_RETRACTING)
					{
						if (grabbers[grabType].index == grabIndex)
						{
							//nothing
						}
						else
						{
							grabbers[grabType].state = GrabberState_RETRACTING_RESTART;
							grabbers[grabType].next_index = grabIndex;
						}
					}
					else if (grabbers[grabType].state == GrabberState_RETRACTING_RESTART)
					{
						grabbers[grabType].next_index = grabIndex;
					}
				}
			}
		}
			break;
			
		case TYPESETUP:
		{
			// example - light the setup LED
			hal_plot_led(TYPESETUP, 0, value, value, value);
		}
			break;
	}
}

void app_surface_event(u8 type, u8 padIndex, u8 value)
{
	if (game_state == GameState_INIT)
	{
		game_state = GameState_RUNNING;
		update_score_LEDs();
		update_LEDs();
	}
	else if (game_state == GameState_RUNNING)
	{
		handle_input(type, padIndex, value);
	}
}


//______________________________________________________________________________
//
// MIDI
//______________________________________________________________________________

void app_midi_event(u8 port, u8 status, u8 d1, u8 d2)
{
	// example - MIDI interface functionality for USB "MIDI" port -> DIN port
	if (port == USBMIDI)
	{
		hal_send_midi(DINMIDI, status, d1, d2);
	}

	// // example -MIDI interface functionality for DIN -> USB "MIDI" port port
	if (port == DINMIDI)
	{
		hal_send_midi(USBMIDI, status, d1, d2);
	}
}

void app_cable_event(u8 type, u8 value)
{
    // example - light the Setup LED to indicate cable connections
	if (type == MIDI_IN_CABLE)
	{
		hal_plot_led(TYPESETUP, 0, 0, value, 0); // green
	}
	else if (type == MIDI_OUT_CABLE)
	{
		hal_plot_led(TYPESETUP, 0, value, 0, 0); // red
	}
}

//______________________________________________________________________________
//
// Timers
//______________________________________________________________________________

void app_timer_event()
{
	static int completionTimer = 0;
	
	if (game_state == GameState_RUNNING)
	{
		for (int i = 0 ; i < GrabType_NBR_ELT ; i++)
		{
			if (grabbers[i].tick >= 0)
			{
				grabbers[i].tick++;
				
				if (grabbers[i].tick > GRABBER_TICKS)
				{
					grabbers[i].tick = 0;
					
					switch (grabbers[i].state)
					{
						case GrabberState_STOPPED:
						{
							break;
						}
						case GrabberState_EXTENDING:
						{
							ProgressResult result = can_progress((GrabType)i);
							if (result == ProgressResult_OK)
							{
								grabbers[i].length++;
							}
							else if (result == ProgressResult_COMPLETE)
							{
								score(grabbers[i].type);
								
								if (((scores[GrabType_TOP] >= 8) ||
									  (scores[GrabType_BOTTOM] >= 8) ||
									  (scores[GrabType_LEFT] >= 8) ||
									  (scores[GrabType_RIGHT] >= 8)))
								{
									/* Nb. This -- a return function in the middle of a function -- is horrible.
									I'll refactor at some point in the future! */
									
									game_complete(grabbers[i].type);
									
									return;
								}
								
								setup_nugget();
								grabbers[i].state = GrabberState_RETRACTING;
								grabbers[i].tick = GRABBER_TICKS;
							}
							else
							{
								grabbers[i].state = GrabberState_RETRACTING;
								grabbers[i].tick = GRABBER_TICKS;
							}
							break;
						}
						case GrabberState_RETRACTING:
						{
							if (grabbers[i].length > 0)
							{
								grabbers[i].length--;
							}
							
							if (grabbers[i].length == 0)
							{
								grabbers[i].state = GrabberState_STOPPED;
								grabbers[i].tick = -1;
							}
							break;
						}
						case GrabberState_RETRACTING_RESTART:
						{
							if (grabbers[i].length > 0)
							{
								grabbers[i].length--;
							}
							
							if (grabbers[i].length == 0)
							{
								grabbers[i].index = grabbers[i].next_index;
								grabbers[i].state = GrabberState_EXTENDING;
							}
							break;
						}
					}
					
					update_cells();
					update_LEDs();
				}
			}
		}
	}
	else if (game_state == GameState_INIT)
	{
		reset_cells();
		setup_nugget();
	}
	else if (game_state == GameState_GAME_COMPLETE)
	{
		completionTimer ++;
		
		if (completionTimer > COMPLETION_TICKS)
		{
			reset_game();
			update_LEDs();
			update_score_LEDs();
			completionTimer = 0;
		}
	}
}


//______________________________________________________________________________
//
// Other functions
//______________________________________________________________________________

ProgressResult can_progress(GrabType type)
{
	ProgressResult result = ProgressResult_OK;
	
	int next_row = 0;
	int next_col = 0;
	
	switch (type)
	{
		case GrabType_TOP:
		{
			next_col = grabbers[type].index;
			next_row = 7 - grabbers[type].length;
			break;
		}
		case GrabType_BOTTOM:
		{
			next_col = grabbers[type].index;
			next_row = grabbers[type].length;
			break;
		}
		case GrabType_LEFT:
		{
			next_col = grabbers[type].length;
			next_row = grabbers[type].index;
			break;
		}
		case GrabType_RIGHT:
		{
			next_col = 7 - grabbers[type].length;
			next_row = grabbers[type].index;
			break;
		}
		default:
		{
			break;
		}
	}
	
	if ((next_row < 0) || (next_row >= NUM_ROWS) || (next_col < 0) || (next_col >= NUM_COLUMNS))
	{
		result = ProgressResult_BLOCKED;
	}
	else
	{
		CellType type = cells[next_row][next_col];
		
		if (type == CellType_EMPTY)
		{
			result = ProgressResult_OK;
		}
		else if (type == CellType_NUGGET)
		{
			result = ProgressResult_COMPLETE;
		}
		else
		{
			result = ProgressResult_BLOCKED;
		}
	}
	
	return result;
}

void score(GrabType grabType)
{
	scores[grabType]++;
	update_score_LEDs();
}

void game_complete(GrabType grabType)
{
	for (int row = 0 ; row < NUM_ROWS ; row++)
	{
		for (int col = 0 ; col < NUM_COLUMNS ; col++)
		{
			u8 index = (row+1)*10 + (col+1);
			hal_plot_led(TYPEPAD, index, colors[grabType].r, colors[grabType].g, colors[grabType].b);
		}
	}
	
	game_state = GameState_GAME_COMPLETE;
	
	scores[0] = 0;
	scores[1] = 0;
	scores[2] = 0;
	scores[3] = 0;
}


//______________________________________________________________________________
//
// Reset methods
//______________________________________________________________________________

void reset_cells()
{
	for (int row = 0 ; row < NUM_ROWS ; row++)
	{
		for (int col = 0 ; col < NUM_COLUMNS ; col++)
		{
			cells[row][col] = CellType_EMPTY;
		}
	}
}

void reset_grabbers()
{
	for (int i = 0 ; i < GrabType_NBR_ELT ; i++)
	{
		grabbers[i].index = 0;
		grabbers[i].next_index = 0;
		grabbers[i].state = GrabberState_STOPPED;
		grabbers[i].length = 0;
		grabbers[i].type = (GrabType)i;
		grabbers[i].tick = -1;
	}
}

void reset_game()
{
	game_state = GameState_INIT;
	reset_grabbers();
	reset_cells();
}


//______________________________________________________________________________
//
// Update methods
//______________________________________________________________________________


void update_cells()
{
	reset_cells();
	
	for (int length = 0; length < grabbers[GrabType_TOP].length; length++)
	{
		u8 row = 7 - length;
		u8 col = grabbers[GrabType_TOP].index;
		cells[row][col] = CellType_GRABBER_TOP;
	}
	
	for (int length = 0; length < grabbers[GrabType_BOTTOM].length; length++)
	{
		u8 row = length;
		u8 col = grabbers[GrabType_BOTTOM].index;
		cells[row][col] = CellType_GRABBER_BOTTOM;
	}
	
	for (int length = 0; length < grabbers[GrabType_LEFT].length; length++)
	{
		u8 row = grabbers[GrabType_LEFT].index;
		u8 col = length;
		cells[row][col] = CellType_GRABBER_LEFT;
	}
	
	for (int length = 0; length < grabbers[GrabType_RIGHT].length; length++)
	{
		u8 row = grabbers[GrabType_RIGHT].index;
		u8 col = 7 - length;
		cells[row][col] = CellType_GRABBER_RIGHT;
	}
	
	cells[nugget_row][nugget_col] = CellType_NUGGET;
}

void update_LEDs()
{
	for (int row = 0 ; row < NUM_ROWS ; row++)
	{
		for (int col = 0 ; col < NUM_COLUMNS ; col++)
		{
			CellType type = cells[row][col];
			
			u8 r = 0;
			u8 g = 0;
			u8 b = 0;
			
			switch (type)
			{
				case CellType_EMPTY:
				{
					break;
				}
				case CellType_NUGGET:
				{
					r = MAXLED;
					g = MAXLED;
					b = MAXLED;
					break;
				}
				case CellType_GRABBER_TOP:
				{
					r = colors[GrabType_TOP].r;
					g = colors[GrabType_TOP].g;
					b = colors[GrabType_TOP].b;
					break;
				}
				case CellType_GRABBER_BOTTOM:
				{
					r = colors[GrabType_BOTTOM].r;
					g = colors[GrabType_BOTTOM].g;
					b = colors[GrabType_BOTTOM].b;
					break;
				}
				case CellType_GRABBER_LEFT:
				{
					r = colors[GrabType_LEFT].r;
					g = colors[GrabType_LEFT].g;
					b = colors[GrabType_LEFT].b;
					break;
				}
				case CellType_GRABBER_RIGHT:
				{
					r = colors[GrabType_RIGHT].r;
					g = colors[GrabType_RIGHT].g;
					b = colors[GrabType_RIGHT].b;
					break;
				}
				default:
				{
					break;
				}
			}
			
			u8 index = (row+1)*10 + (col+1);
			
			hal_plot_led(TYPEPAD, index, r, g, b);
		}
	}
}

void update_score_LEDs()
{
	for (int i = 0 ; i < 8 ; i++)
	{
		u8 r = 0;
		u8 g = 0;
		u8 b = 0;
		
		if (scores[GrabType_BOTTOM] > i)
		{
			r = colors[GrabType_BOTTOM].r;
			g = colors[GrabType_BOTTOM].g;
			b = colors[GrabType_BOTTOM].b;
		}
		
		hal_plot_led(TYPEPAD, i+1, r, g, b);
	}
	
	for (int i = 0 ; i < 8 ; i++)
	{
		u8 r = 0;
		u8 g = 0;
		u8 b = 0;
		
		if (scores[GrabType_TOP] > i)
		{
			r = colors[GrabType_TOP].r;
			g = colors[GrabType_TOP].g;
			b = colors[GrabType_TOP].b;
		}
		
		hal_plot_led(TYPEPAD, i+91, r, g, b);
	}
	
	for (int i = 0 ; i < 8 ; i++)
	{
		u8 r = 0;
		u8 g = 0;
		u8 b = 0;
		
		if (scores[GrabType_LEFT] > i)
		{
			r = colors[GrabType_LEFT].r;
			g = colors[GrabType_LEFT].g;
			b = colors[GrabType_LEFT].b;
		}
		
		hal_plot_led(TYPEPAD, 10*(i+1), r, g, b);
	}
	
	for (int i = 0 ; i < 8 ; i++)
	{
		u8 r = 0;
		u8 g = 0;
		u8 b = 0;
		
		if (scores[GrabType_RIGHT] > i)
		{
			r = colors[GrabType_RIGHT].r;
			g = colors[GrabType_RIGHT].g;
			b = colors[GrabType_RIGHT].b;
		}
		
		hal_plot_led(TYPEPAD, 10*(i+1)+9, r, g, b);
	}
}