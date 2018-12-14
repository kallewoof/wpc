# WPC
Weighted Prioritized Combinator

WPC is a tool for maintaining a set of test options of varying importance. It includes a simple language (`wpc`), a compiler (which turns `.wpc` files into test parameter instances), and an executor (which generates test parameters based on a parameter instance).

WPC is written to be used in a testing environment where there are so many combinations that testing everything every time is unfeasible. Due to the stateful nature of parameter instances, tests can be "resumed" between code updates, and combinations of parameters are tested in an order that optimizes the variety of combinations; test maximum amount of stuff in minimum amount of time!

Quick example:
```
# file: cow.wpc
cow {
  normal value green;
  uninteresting value white;
}
```
Compile:
```Bash
$ wpc cow.wpc
2
```
(Let's ignore the interest stuff for now.) We get a 2 back. That tells us the number of combinations there are in total. In our case, there are two combinations:

* `cow=green`
* `cow=white`

The call to `wpc` generated a `cow.scd` file. This is an *instance* of the cow test above; we can derive parameters for testing from this using the `wpx` command:
```Bash
$ wpx cow.scd
cow=green
$ wpx cow.scd
cow=white
$ wpx cow.scd
$ 
```

Let's briefly talk about the interest part now. First let's try this again with the values flipped:
```
# file: cow2.wpc
cow2 {
  uninteresting value white;
  normal value green;
}
```
If we run this one we get:
```Bash
$ wpx cow2.scd
cow2=green
$ wpx cow2.scd
cow2=white
$ wpx cow2.scd
$ 
```
I.e. the exact same output as above. While there is some randomness (called "blurring" here), parameters are in fact ordered in a fixed manner.

There are currently 6 prioritizers for values: heavy (-1), uninteresting (-1), normal (0), light (+1), interesting (+1), prioritized (+1). These can be combined any number of times, e.g.
```
heavy uninteresting prioritized value x;
```
(would produce a value `x` for the given option, whose priority is `-1-1+1=-1`).

The priority of a value determines the order in which the value is enabled. You can list the contents of an instance, and see priorities for each configuration, by prepending the `wpx` call with `-l` or `--list`.
```Bash
$ wpc cow.wcd
2
$ wpx -l cow.scd
 #  | PRI      | CONFIG
  1 | -0.01590 | cow=white
  0 |  1.02936 | cow=green
$ wpc cow2.wpc
2
$ wpx -l cow2.scd
 #  | PRI      | CONFIG
  1 |  0.02938 | cow2=white
  0 |  1.02610 | cow2=green
$ 
```
(the outputs differ, not because `cow.wcd` is different from `cow2.wcd`, but because the starting priorities are blurred; try compiling and listing multiple times and you'll note that each time is slightly different)

## Configuration priority

When emitting the next configuration to test, WPC makes use of priorities to judge which parameters have been tested the least, or which configuration is *the most interesting* at this point in time.

This is achieved by doing two things:
* firstly, by generating a configuration priority for all possible combinations based on the priorities of their individual selections
* secondly, by modifying the configuration priority for all remaining combinations, so that combinations whose selections have been tested are decreased

To see how this works we need to expand our cow test to include a ferret:
```
# file: animals.wpc
cow {
  normal value green;
  uninteresting value white;
}
ferret {
  normal value squirrely;
  normal value slithery;
}
```
Compile:
```Bash
$ wpc animals.wpc
4
$ wpx -l animals.scd
 #  | PRI      | CONFIG
  3 | -0.04137 | cow=white, ferret=squirrely
  2 |  0.01922 | cow=white, ferret=slithery
  1 |  0.95447 | cow=green, ferret=slithery
  0 |  1.02949 | cow=green, ferret=squirrely
```
We now have four combinations: green cow and squirrely ferret, green cow and slithery ferret, white cow and slithery ferret, and white cow and squirrely ferret.

If we emit a configuration WPC will note that we have now tested green cows once, and will subsequently lower the priority of *all green cow configurations*. It will similarly lower the priority for all squirrely ferret configurations, since we will have tested that once, as well:
```Bash
$ wpx animals.scd
cow=green
ferret=squirrely
$ wpx -l animals.scd
 #  | PRI      | CONFIG
  2 | -0.05137 | cow=white, ferret=squirrely
  1 |  0.01922 | cow=white, ferret=slithery
  0 |  0.94447 | cow=green, ferret=slithery
$
```
In this case, the ordering did not change, but you may note that the priority for `cow=white, ferret=squirrely` has decreased from `-0.04137` to `-0.05137`, because of the fact we have already tested `ferret=squirrely` once. The same of course applies to `cow=green, ferret=slithery`, going from `0.95447` to `0.94447`, because we have tested `cow=green` once.

Note: the `wpx` command will return an exit code `0` if a configuration was emitted successfully, and an exit code `1` if there were no configurations left to test.

## Simple bash script example

```Bash
#!/bin/bash
# Test cows and ferrets
# If user writes "resume" as first argument, we resume from the existing file otherwise
# we restart
if [ "$1" != "resume" ]; then
    echo "Generating schedule"
    wpc animals.wpc
fi
while true; do
    wpx -l animals.scd
    wpx animals.scd > instance.rc
    ec=$?
    if [ $ec -eq 1 ]; then
        echo "Finished all instances"
        break
    elif [ $ec -ne 0 ]; then
        echo "wpx call failed!"
        exit 1
    fi
    source instance.rc
    echo "TESTING cow value: $cow   ferret value: $ferret"
    for i in $(seq 3); do sleep 1; echo -n -e ". "; done
    echo " OK"
done
```

## Conditions

> Slithery ferrets require green cows.

WPC includes a simple condition operator for values.

Let's say we have a system which may or may not use a database backend (`db=mysql`, `db=none`).
We have a series of database related properties, such as `db_location=remote`, `db_location=local`,
which are only relevant if we actually *have* a database in the first place.
WPC can be told that a specific value depends on some other value in some manner:
```
# file: animals.wpc
cow {
  normal value green;
  uninteresting value white;
}
ferret {
  normal value squirrely;
  normal value slithery(require cow==green);
}
```
When we compile this:
```
$ wpc animals.wpc
3
```
we only get 3 alternatives, not 4:
```
$ wpx -l animals.scd
 #  | PRI      | CONFIG
  2 |  0.01198 | cow=white, ferret=squirrely
  1 |  0.99974 | cow=green, ferret=slithery
  0 |  1.03089 | cow=green, ferret=squirrely
$ 
```
To take the example above:
```
# file: websrv.wpc
db {
  uninteresting value none;
  normal value mysql;
}
db_location {
  normal value remote(require db!=none);
  normal value local(require db!=none);
  normal value none(require db==none);
}
```
```Bash
$ wpc websrv.wpc
3
$ wpx -l websrv.scd
 #  | PRI      | CONFIG
  2 |  0.03105 | db=none, db_location=none
  1 |  0.97296 | db=mysql, db_location=local
  0 |  1.00419 | db=mysql, db_location=remote
$ 
```

## Extra output

By default, WPC will only output `option=value` for each option. Sometimes you want to include additional output based on selected values, such as database hosts or such:

```
# file: websrv.wpc
db {
  uninteresting value none;
  normal value mysql;
}
db_location {
  normal value remote(require db!=none) {{
    db_host=remote-server.somewhere
  }}
  normal value local(require db!=none) {{
    db_host=localhost
  }}
  normal value none(require db==none);
}
```
```Bash
$ wpc websrv.wpc
3
$ wpx websrv.scd
db=mysql
db_location=local
    db_host=localhost

$ wpx websrv.scd
db=mysql
db_location=remote
    db_host=remote-server.somewhere

$ wpx websrv.scd
db=none
db_location=none
$ 
```

Anything between the double curlies will be emitted as is, and may include code snippets or such (the only restriction is that it must not contain "`}}`", as that will end the sequence):

```
# file: websrv.wpc
db {
  uninteresting value none;
  normal value mysql;
  normal value mongodb;
}
db_location {
  normal value remote(require db!=none) {{
    if [ "$db" = "mongodb" ]; then
        db_host=remote-mongo-server.somewhere
    else
        db_host=remote-server.somewhere
    fi
  }}
  normal value local(require db!=none) {{
    db_host=localhost
  }}
  normal value none(require db==none);
}
```
```Bash
$ wpx websrv.scd
db=mongodb
db_location=local
    db_host=localhost

$ wpx websrv.scd
db=mongodb
db_location=remote
    if [ "$db" = "mongodb" ]; then
        db_host=remote-mongo-server.somewhere
    else
        db_host=remote-server.somewhere
    fi

$ wpx websrv.scd
db=mysql
db_location=remote
    if [ "$db" = "mongodb" ]; then
        db_host=remote-mongo-server.somewhere
    else
        db_host=remote-server.somewhere
    fi

$ wpx websrv.scd
db=mysql
db_location=local
    db_host=localhost

$ wpx websrv.scd
db=none
db_location=none
$ 
```
