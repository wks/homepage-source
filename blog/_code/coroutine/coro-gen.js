function* traverse(x) {
    if (Array.isArray(x)) {
        for (const elem of x) {
            for (const y of traverse(elem)) {
                yield y;  // Yield what the inner layer yields.
            }
        }
    } else {
        yield x;
    }
}

const DATA = [1, [[2, 3], [4, 5]], [6, 7, 8]];

let gen = traverse(DATA);
for (;;) {
    const result = gen.next();  // Resumes the coroutine.
    if (result.done) {
        break;
    }
    console.log(result.value);
}
