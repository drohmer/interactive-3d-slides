# Writing style of the tutorial

The pages follow the style of the lab pages of Damien Rohmer (EPITA Ani3D, CSC 43043 at
Ecole polytechnique): a teacher guiding a student through a program, step by step.

## Structure of a page

- At the top, the path of the code in gray: `<div class="code-path">Code: examples/a1-opengl-cube/</div>`.
- First sentence: the objective. "The objective of this example is to ...", "In this example we ...".
- Then the expected result: "Once built, you should obtain ...", with the live demo just after.
- Then the initial state or the organization of the files, as a list `file`: what it contains.
- Sections are short noun phrases: "Build for the web", "Adding a panel", "Buttons in the slide".

## Actions and explanations

- Each thing to do is a list item that starts with a bold `**&gt;**` and an imperative verb:
  `* **&gt;** Build the desktop version, and run it.` The key part of the action can be in bold.
- After an action, what to observe: "You should see ...", "Check that ...", "Make sure that ...".
- Explanations go in sub-items starting with "- ": what a function does, where a variable is, why.
- Code is introduced by a sentence ending with "the following code" / "with the following line", and
  kept short; long files are included in full after the explanation.
- When code is explained element by element: the element in bold or as `code`, then sub-items.

## Remarks

- Labels: "Note:", "Rem.", "Reminder:", "Hints:" (French: "Note :", "Rem.", "Rappel :", "Indications :").
- Small precisions in italics and parentheses: *(some details may vary)*, *(see also the README)*.
- Reassure on expected limits: "It is normal that ...", "Usually you won't have to edit these files."

## Tone

- Plain, technical, direct; "you" for the reader, "we" for what the page does ("we now add ...").
- Complete sentences, sometimes long, with precisions in parentheses; no slogans, no marketing.
- English first; the French version is written in the same style, not word for word
  ("On rappelle les éléments suivants", "Par ex.", "Rem.", vous).
