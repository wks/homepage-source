import greenlet

def traverse(x):
    if isinstance(x, list):
        for elem in x:
            traverse(elem)
    else:
        greenlet.getcurrent().parent.switch(x)  # switch to parent
        
def greenlet_traverse(x):
    def _traverse_x():
        traverse(x)
        raise StopIteration()
    return greenlet.greenlet(_traverse_x)  # The parent is the current greenlet

DATA = [1, [[2, 3], [4, 5]], [6, 7, 8]]

glet = greenlet_traverse(DATA)
try:
    while True:
        v = glet.switch()   # switch to the greenlet
        print(v)
except StopIteration:
    pass
