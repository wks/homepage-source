import greenlet

def traverse(x, parent):
    if isinstance(x, list):
        for elem in x:
            traverse(elem, parent)      # recursive call
    else:
        parent.switch(x)                # can switch at any level of stack
        
def greenlet_traverse(x):
    current = greenlet.getcurrent()     # remember the current coroutine
    def _traverse_x():
        traverse(x, current)            # and pass it as the parent
        current.throw(StopIteration)
    return greenlet.greenlet(_traverse_x)

DATA = [1, [[2, 3], [4, 5]], [6, 7, 8]]

glet = greenlet_traverse(DATA)
try:
    while True:
        v = glet.switch()   # switch to the greenlet
        print(v)
except StopIteration:
    pass
