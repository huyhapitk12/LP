class Entity:
    x: float = 0.0
    y: float = 0.0

    def move(self, dx: float, dy: float) -> float:
        self.x = self.x + dx
        self.y = self.y + dy
        return self.x

class Player(Entity):
    name: str = "Ranger"
    health: int = 100
    potions: int = 2

    def take_damage(self, amount: int) -> int:
        self.health = self.health - amount
        return self.health

    def use_potion(self) -> int:
        if self.potions > 0:
            self.potions = self.potions - 1
            self.health = self.health + 20
        return self.health

class Enemy(Entity):
    name: str = "Goblin"
    damage: int = 12

    def attack(self, target: Player) -> int:
        return target.take_damage(self.damage)

class Boss(Enemy):
    rage_bonus: int = 8

    def attack(self, target: Player) -> int:
        return target.take_damage(self.damage + self.rage_bonus)

def main():
    hero = Player()
    hero.name = "Aria"

    scout = Enemy()
    scout.name = "Scout Goblin"
    scout.damage = 9

    boss = Boss()
    boss.name = "Stone Ogre"
    boss.damage = 18
    boss.rage_bonus = 6

    hero.move(2.0, 1.0)
    scout.move(2.0, 1.0)
    boss.move(5.0, 1.0)

    print("Hero health before combat:")
    print(hero.health)

    print("Scout attacks first.")
    scout.attack(hero)
    print(hero.health)

    print("Hero drinks a potion.")
    hero.use_potion()
    print(hero.health)

    print("Boss attacks.")
    boss.attack(hero)
    print(hero.health)

main()
