# A training round for `mixtrain build` (see the README: "Train a model of your own"). '#' starts a comment.
round 1
base ../../models/automix-v1.2.0.model      # the reference: the model you start from (a path from this file, or from where you run it)
seed 1                                      # any number: another seed picks other joins
exclude rounds                              # never a pair already heard in a round.json under this folder

# how many joins of each kind (drop, bridge, structure, beat, fade); each becomes three demos: the base and two versions
joins drop 2
joins beat 3
joins bridge 2

# each kind's two versions: the numbers they change from the base (any number of a model file, and gateOn)
version drop 1 exitBars 2
version drop 2 gateEvery 2
version beat 1 blendFrom 16
version beat 2 gate 1 gateLevel 1 gateDuck 0.3 gateEvery 2 gateOn beat
version bridge 1 tempoBridgeMax 4
version bridge 2 bridgeOutPow 1

# a join heard before, again: its two paths, or their ends ("Album/01 - Song.flac")
# retest Album/01 - Song.flac | Album/02 - Other Song.flac
# used instead of a version when, on a join, it would sound the same as the reference or the version before it
# alternative beat 1 gate 1 gateLevel 1 gateDuck 0.3 gateOn beat
