# Object Oriented Programming Demo
class Entity:
    x: float = 0.0
    y: float = 0.0
    
    def move(self, dx: float, dy: float) -> float:
        self.x = self.x + dx
        self.y = self.y + dy
        return self.x

class Player(Entity):
    health: int = 100
    name: str = "Hero"
    
    def take_damage(self, amount: int) -> int:
        self.health = self.health - amount
        return self.health
        
    def profile(self) -> str:
        return self.name

class Enemy(Entity):
    damage: int = 15
    
    def attack(self, target: Player) -> int:
        return target.take_damage(self.damage)

def main():
    p = Player()
    p.name = "Arthur"
    
    e = Enemy()
    e.damage = 25
    
    e.move(10.5, 5.0)
    p.move(10.5, 5.0)
    
    print("Player Health initially:")
    print(p.health)
    
    print("Enemy Attacks!")
    e.attack(p)
    
    print("Player Health after:")
    print(p.health)

main()
