Project 4: Snake Game#
Click here to download the reference implementation bin file for homework 4.

As the CS110 course draws to a close, it is time to put what you have learned in this course into practice. In this project, you will combine C language and RISC-V assembly language to implement a classic Snake game. You will utilize the PlatformIO environment for cross-compilation and flash the program onto the Longan Nano development board to run. Although we provide a reference implementation, you are not required to replicate it exactly. Once the basic functional requirements specified in the document are met, you are highly encouraged to add or improve features based on your own creativity (e.g., increasing level difficulty, adding special items, or optimizing visual effects). We hope you enjoy the fun of the development on the Longan Nano in this project.

Important Notes
Important Note 1: This year's Project 4 is an individual project — it must be completed independently by each individual, and teaming up is not allowed.
Important Note 2: We only grade the logic and functionality related to the Snake game. You may develop other mini-games out of interest, but they will not be counted towards the final score.
Important Note 3: Project 4 will be graded by TAs through manual check-offs during Lab sessions; no Autograder will be used. Please refer to the "Grading Policy" section for specific grading details.
Academic Integrity Policy
We will perform similarity detection on the submitted code. If your code (excluding the base Lab template provided in the course) is too similar to others, you will face severe penalties from the entire TA team and both professors.

Grading Policy
The total score for Project 4 is 20 points. Please note that if your code fails to compile or assemble, this project will receive 0 points. However, the good news is that in Project 4, we will not perform memory leak checks, nor will we enable additional compiler warning flags such as -Wall and -Werror.

Since the check-off process for Project 4 is relatively complex, it may not be possible to finish inspecting all students in a class during regular Lab hours. Therefore, please be prepared for a longer project check-off time. Project 4 grading is performed manually by TAs during the check-off process; no Autograder is used.

Check-off Location and Time: Same as your regular Lab location.
Check-off Process After Deadline: On the student's computer, retrieve your submitted Project 4 source code from Gradescope, then compile and flash it onto the development board on-site to run.
Grading Mechanism: All requirements adopt an "all-or-nothing" grading mechanism. For example, you will only receive 0 or 2 points, with no intermediate scores.
Implementation and Submission Policy
Project 4 does not have a fixed code template, but this also means fewer constraints. You will implement the entire logic of Project 4 from scratch. You can start with the Lab 11 template ("lab-longan-nano-starter-20250423"), and you are allowed to:

Modify any code in the template.
Add or delete any .c or .h source files to make your project structure clearer.
Submission Requirements
Your submission must contain at least:

All basic files from the Lab 12 template (unless you deem certain files no longer necessary and delete them).
All new source code (.c, .h, .S) you added, as well as related resource files (such as images or font data).
Detailed Implementation Requirements
Your specific implementation should be similar in functionality and visual presentation to the Reference Implementation, but we do not require them to be exactly identical. For all requirements not explicitly specified in this lab document, please refer to the actual running behavior of the Reference Firmware. If there are vague or ambiguous statements in the document, please check the posts on Piazza first; if your question remains unresolved, please post a question on Piazza.

1. Basic Functionalities
This section covers the basic gameplay and logical requirements of the Snake game.

1.1 Mandatory RISC-V Requirement: Scoreboard Implementation
*The scoreboard records the level names and scores of the top three best games.*

You must use RISC-V assembly language to implement the logic of the historical scoreboard. The specific requirements are as follows:

Assembly Implementation Content: The recording of scores and the ranking display must be completed in the .S file. Within the assembly logic, you can call C functions such as LCD drivers or button inputs (e.g., LCD_ShowString, Get_Button), but it is strictly forbidden to call any C functions that contain core business logic (such as score comparison, sorting algorithms, or data processing).
Persistent Records: The scoreboard should retain records across multiple game sessions without manually pressing the development board's Reset button, but it will be cleared after a hard reset.
Automatic Sorting Logic: The scoreboard needs to maintain the top three scores in real time and sort them in descending order according to the following priorities:
Primary Criterion: Sorted by score from highest to lowest.
Secondary Criterion: If the scores are identical, sorted by level difficulty (e.g., 1-3 > 1-2) from highest to lowest.
Trigger Timing: After each game ends (death), the program should call the assembly function to update the scoreboard, even if the player chooses to return directly to the level selection interface instead of entering the scoreboard view.
Interaction & Switching: Pressing the joystick middle button after death calls the assembly function to enter the scoreboard interface and view rankings. Pressing SW1 on the scoreboard interface leaves the scoreboard and returns to the difficulty/level selection interface.
• [Correct interface switching: 1 pt]
• [Persistent records (even without entering the scoreboard): 1 pt]
• [Correct sorting: 1 pt]
• No points will be awarded if RISC-V assembly is not used for this part.

1.2 UI & Controls
Default Menu Interface: The game must enter a menu interface upon startup. Although only level "1-1" is required to be implemented in the basic part, the interface must contain at least three selectable options: 1-1, 1-2, and 1-3. The currently selected option must be highlighted in color (even if levels 1-2 and 1-3 are not fully implemented).
• [Correct interface representation (including the cursor for the currently selected item): 1 pt]
Interface Switching:
Enter Level: Press the joystick middle button when any level is selected to enter that level.
Forced Exit/Return: Pressing the SW1 button during gameplay or on the scoreboard interface should immediately stop the current logic and return to the difficulty selection menu.
• [Correct level switching and exiting: 1 pt]
1.3 Snake Logic
Direction Control: Use the joystick to control the snake's movement (up, down, left, right).
Movement & Growth: The snake moves at a constant initial speed. Each time it eats a gold coin (item), the snake's body length increases by one grid, and 1 point is added.
High-speed Movement: When the SW2 button is pressed, the snake enters high-speed movement mode (the speed doubles).
Death Condition: When the snake's head hits the screen edge (wall) or crashes into its own body, the game ends, entering the death settlement process and recording the score.
1.4 Items System: Gold/Collectibles
Random Spawning: Gold coins should be randomly generated in empty areas of the map.
Time Limit: Each gold coin only exists for 10 seconds after spawning. If the player fails to eat it within 10 seconds, the gold coin disappears and a new spawn timer resets.
1.5 Real-time Score Counter
During gameplay, a score counter must be set up in an appropriate area on the screen to display the current session's score in real time.
• [Correct control, movement, and high-speed movement: 1 pt]
• [Correct gold coin interaction (scoring and length growth): 1 pt]
• [Correct death condition (walls and self-collision): 1 pt]
• [Correct gold coin spawning and disappearance: 1 pt]
• [Correct score counter: 1 pt]

2. Level Richness Enhancement
In this section, you need to add two independent levels with more complex logic on top of the basic functionalities. These two levels should serve as parallel options on the difficulty selection interface for players to choose (corresponding to 1-2 and 1-3).

2.1 Level 1-2: Walls, Gems & Portals
This level focuses on the logical handling of environmental interactions and terrain obstacles.

Static Obstacles (Walls): At the start of the game, generate at least four 1*1 walls on the map (you are allowed to plan their positions and quantities yourself).
Collision Logic: The physical properties of these walls are identical to the outer boundary walls; hitting them results in an immediate game over.
Spawning Constraints: The initial positions of gold coins, gems, portals, and the snake's body must not overlap with these walls.
Spatial Teleportation (Portals): Generate a pair of (two) portals on the map.
Teleportation Logic: The portals are bi-directionally connected. When the snake's head enters one portal, it will immediately emerge from the position and direction corresponding to the other portal.
Advanced Items: Gems:
Spawning Probability: Items generated on the field have a 50% probability of being designated as gems (they must be identified using a color or pattern distinct from gold coins).
Rewards: Gems only exist for 5 seconds. Eating a gem awards 2 points, and the snake's body length increases by one grid.
• [Correct wall generation and related interaction: 1 pt]
• [Correct portal generation and interaction: 1 pt]
• [Correct gem generation and interaction: 1 pt]

2.2 Level 1-3: Enemies, AI Logic & Spawning
This level is parallel to Level 1-2 (meaning it does not include extra walls, portals, or gems) and focuses on the implementation of adversarial logic.

Adversarial Snake (AI Snake): At the start of the game, a machine-controlled snake is generated at the symmetrical point of the player's initial position.
Fixed Length: The enemy snake's length is fixed at 3 grids (therefore, you do not need to handle its self-collision logic).
Behavioral Logic:
Target-driven: When a gold coin is on the field, the enemy snake actively moves towards the gold coin's position.
Auto-cruise: When there are no gold coins on the field, it maintains its current direction and cruises in a straight line.
Mandatory Obstacle Avoidance: It must turn when it is 1 grid away from a wall (excluding the player's snake); it is uniformly regulated that it always prioritizes turning right.
Collision & Gameplay Logic:
Player Condition: If the player's snake head collides with the enemy's body, it is treated as hitting a wall, resulting in a game over.
Enemy Condition: If the enemy's snake head collides with the player-controlled snake body, the enemy snake dies.
Loot Drop: Upon death, the enemy snake must leave 3 gold coins on the spot.
• [Correct enemy generation and auto-cruise logic: 1 pt]
• [Correct enemy-gold interaction logic: 1 pt]
• [Correct enemy collision, death, and drop logic: 1 pt]

3. Overall Experience Enhancement
To enhance the completeness and user experience of the game, you need to conduct deep optimization in display logic, operational stability, and visual performance. Complete at least two items from Section 3.1 to 3.3. If Section 3.3 is chosen, at least two animations must be completed.

• [2 pts per item, up to 4 pts maximum]

3.1 Flicker-Free Rendering
Screen Flicker Improvement: If you redraw all graphics every time, you will notice a distinct flickering process whenever the screen updates, which is harmful to the player's vision. You can eliminate this flickering by only altering the pixels that need to be updated.

Incremental Update Logic: You must improve the refresh mechanism to "only update pixels that have changed". For instance, when the snake moves, you should only paint the grid where the snake is, instead of refreshing the entire screen.
3.2 Fixed FPS Control
Fixed FPS: Since the computational load of each round of logic updates (such as AI pathfinding, collision detection, and portal judgment) is not completely identical, if no limits are applied and the screen updates immediately upon calculation completion, the latency of each screen update will vary slightly. This subtle fluctuation leads to a visual "jittery sensation".

You must limit the refresh frequency by setting a fixed time period. Regardless of whether the logic calculation of the current frame is fast or slow, the start time interval of each frame should be strictly consistent, thereby ensuring the absolute stability of the FPS.
3.3 Custom Visual Animations
To increase the artistic expression and visual friendliness of the game, you can choose at least two of the following options to complete:

Startup/Loading Animation: Before displaying the main menu interface, design a dynamic startup sequence. For example: a fade-in/fade-out effect for the author's name or project logo, an animation of the snake wrapping around the screen, or a loading interface with a progress bar. This splash screen must be dynamic rather than a purely single static screen.
Death Transition Animation: When a collision occurs, do not jump to the settlement interface immediately. Visual effects such as the snake body dissipating block by block from tail to head, or the "Game Over" text shaking into view can be utilized to enhance the sense of interactive feedback.
UI Dynamic Feedback: For example, on the difficulty selection interface, the selected option has a breathing light flashing effect; or when a gold coin is about to disappear (the last 2 seconds), it flashes rapidly to alert the player.
