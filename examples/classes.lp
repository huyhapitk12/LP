# OOP - Classes in LP
# Run: lp classes.lp

class Dog:
    name = "Unknown"
    age = 0

    def bark(self):
        print("Woof!")

    def info(self):
        print(self.name)
        print(self.age)

# Create and use instances
my_dog = Dog()
my_dog.name = "Rex"
my_dog.age = 5
my_dog.info()
my_dog.bark()
