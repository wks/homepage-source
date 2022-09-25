def traverse(x):
    if isinstance(x, list):
        for elem in x:
            yield from traverse(elem)   # Use "yield from" to yield everything.
    else:
        yield x

DATA = [1, [[2, 3], [4, 5]], [6, 7, 8]]

for v in traverse(DATA):
    print(v)
