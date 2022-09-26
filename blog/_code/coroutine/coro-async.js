// A zero-capacity queue.
// `enqueue` will block until another task calls `dequeue`,
// and `dequeue` will block until another task calls `enqueue`.
class ZeroQueue {
    constructor() {
        this.getter_resolver = null
        this.data = null
        this.setter_resolver = null
    }

    async enqueue(num) {
        if (this.getter_resolver != null) {
            // If a consumer came before, we satisfy it.
            this.getter_resolver(num)
            this.getter_resolver = null
        } else {
            // If we come first, we leave the value and wait until consumed.
            this.data = num
            await new Promise((resolve) => {
                this.setter_resolver = resolve
            })
        }
    }

    async dequeue() {
        if (this.setter_resolver != null) {
            // If a producer already came, we take the value and let it continue.
            this.setter_resolver(null)
            this.setter_resolver = null
            return this.data
        } else {
            // If we come first, we wait for the producer.
            return await new Promise((resolve) => {
                this.getter_resolver = resolve
            })
        }
    }
}

const queue = new ZeroQueue()

async function traverse(x) {
    if (Array.isArray(x)) {
        for (const elem of x) {
            await traverse(elem)
        }
    } else {
        // await may potentially yield,
        // giving the user an impression of block-waiting.
        await queue.enqueue(x)
    }
}

async function print_all() {
    for (;;) {
        // await may potentially yield,
        // giving the user an impression of block-waiting.
        const v = await queue.dequeue()
        if (v == "end") {
            return
        } else {
            console.log(v)
        }
    }
}

const DATA = [1, [[2, 3], [4, 5]], [6, 7, 8]]

// The first task traverses the list and signal termination.
traverse(DATA).then(() => {
    queue.enqueue("end")
})

// The second task keep polling till the end.
print_all()
