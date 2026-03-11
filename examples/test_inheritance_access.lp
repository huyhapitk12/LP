class Account:
    def describe(self) -> str:
        return "account"

class PremiumAccount(Account):
    protected tier: str = "premium"
    private internal_code: int = 7

    def describe(self) -> str:
        return self.tier

    protected def get_tier(self) -> str:
        return self.tier

class TeamAccount(PremiumAccount):
    def reveal_tier(self) -> str:
        return self.tier

    def reveal_method(self) -> str:
        return self.get_tier()

def test_inheritance_and_protected_access():
    premium = PremiumAccount()
    team = TeamAccount()

    if premium.describe() != "premium":
        raise "override failed"
    if team.reveal_tier() != "premium":
        raise "protected field inheritance failed"
    if team.reveal_method() != "premium":
        raise "protected method inheritance failed"
