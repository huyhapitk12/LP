# Data Structures - LP Language
# Run: lp data_structs.lp

# Lists
numbers = [1, 2, 3, 4, 5]
print(numbers)

# List Comprehensions
squares = [x * x for x in 10]
print(squares)

evens = [x for x in 20 if x % 2 == 0]
print(evens)

# Dictionaries
person = {"name": "Alice", "age": 30}
print(person)

# Sets (auto-deduplicate)
unique = {1, 2, 3, 2, 1}
print(unique)

# Tuples
point = (10, 20)
print(point)
