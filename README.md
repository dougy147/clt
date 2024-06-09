Concepts learning game inspired by [Mathy and Bradmetz](http://fabien.mathy.free.fr/files/Mathy%20et%20Bradmetz-%202004-%20A%20theory%20of%20the%20graceful%20complexification%20of%20concepts%20and%20their%20learnability.pdf) (2003).

At each level, you need to discover the concept the game expects you to learn, and classify the elements you see on the screen accordingly.

Levels are ordered from least to most difficult.

Works up to 3 dimensions (13 levels).

```console
$ gcc -o concept clt.c -lraylib && ./clt
```

## How to play?

It is mouse playable:

| Command | Action |
|:-------:|:-------:|
| `Left click` | "Yes, the element I see belongs to the concept." |
| `Rigth click` | "No, the element I see does not belong to the concept." |

Your progresses are shown in the progress bar.
A correct answer gives you one point.
Any error makes you go back to zero point.

## What is a concept and how to measure their complexity?

In this task, concepts are _"conjunctions of disjunctive features"_ (shape, size, color).
For example, concepts could be: "square OR red", "(round AND small) OR blue", etc.

The complexity of a concept can be given by the number of "questions" one needs to ask to classify correctly every examples (positive and negative) from the features' space. This measure was made by the authors and used to rank our levels.

## Side note

If one would use this game as a metrics for working memory models experiments, levels should be randomized, time measurement added, and more features (randomized) used to avoid interference between each levels.

# References

- Mathy, F., & Bradmetz, J. (2003). A theory of the graceful complexification of concepts and their learnability. _Current Psychology of Cognition, 22_(1), 41-82. [available here](http://fabien.mathy.free.fr/files/Mathy%20et%20Bradmetz-%202004-%20A%20theory%20of%20the%20graceful%20complexification%20of%20concepts%20and%20their%20learnability.pdf)
