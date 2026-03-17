class MusicQueue {
  constructor() {
    this.items = [];
  }

  enqueue(track) {
    this.items.push(track);
  }

  dequeue() {
    return this.items.shift();
  }

  peek() {
    return this.items[0];
  }

  clear() {
    this.items.length = 0;
  }

  isEmpty() {
    return this.items.length === 0;
  }

  size() {
    return this.items.length;
  }

  toArray() {
    return [...this.items];
  }
}

module.exports = MusicQueue;
