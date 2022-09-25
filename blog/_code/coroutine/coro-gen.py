def traverse(x):
    if isinstance(x, list):
        for elem in x:
            new_gen = traverse(elem)    # Create the next level of generator
            try:
                while True:
                    v = next(new_gen)
                    yield v             # Yield everything the inner generator yields
            except StopIteration:       # until iteration stops.
                pass
    else:
        yield x

DATA = [1, [[2, 3], [4, 5]], [6, 7, 8]]

gen = traverse(DATA)    # The top-level generator
try:
    while True:
        v = next(gen)   # Iterate through it
        print(v)
except StopIteration:   # until iteration stops.
    pass
