# Toastie Cutscenes
An Unreal Engine 5 plugin that imports dialogue written in a text format and converts it into an asset that can be played by a CutscenePlayer actor.

## Toastie Cutscene File
The text format for a Toastie Cutscene is as follows:
```
Scene TestDialogue Dialogue
	Self: "Hello! I am Snoopa 1"
	Snoopa2: "Hello!"
	Snoopa2: "I am Snoopa 2!!"
	Player: "Oh-ho!"
	Player: "Double the peasants, double the tithe!"
	Self: "T-tithe?"
	PlayerChoice
		Option [Listen] "Listen to his excuse"
		Option [Arrest] "Arrest him"
		Option [Kiss] "Kiss him"
		Option Exit "Leave"
	EndPlayerChoice
	
	[Listen]
	Player: "Surely you have it ready?"
	Self: "I can't tell him I dropped it in the sewer..." Think
	Self: "I uh... Have it"
	Self: "..." NoAnimation
	Self: "Somewhere..."
	Exit
	
	[Arrest]
	Player: "Your hesitation is suspect"
	Player: "We'll sort you out in prison"
	Exit
	
	[Kiss]
	Player: "Pucker up"
	Self: "W-What?!"
	Exit
EndScene
```
The following is an example implementation of a Cutscene Player (of the above scene):
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/LRc4_eyuNHs/0.jpg)](https://www.youtube.com/watch?v=LRc4_eyuNHs)
