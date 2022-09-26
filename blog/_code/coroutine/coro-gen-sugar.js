function* traverse(x) {
    if (Array.isArray(x)) {
        for (const elem of x) {
            yield* traverse(elem)
        }
    } else {
        yield x
    }
}

const DATA = [1, [[2, 3], [4, 5]], [6, 7, 8]]

for (const v of traverse(DATA)) {
    console.log(v);
}
