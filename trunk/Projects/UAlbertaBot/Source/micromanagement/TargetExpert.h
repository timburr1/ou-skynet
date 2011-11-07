#ifndef TARGET_EXPERT_H
#define TARGET_EXPERT_H

#include "Properties.h"

class TargetExpert
{
	props::Game	gameProps;
public:
	TargetExpert() : gameProps(BWAPI::Broodwar)
	{

	}

	void Update()
	{
		gameProps = props::Game(BWAPI::Broodwar);
	}

	BWAPI::Unit * SelectTarget(const BWAPI::Unit * attacker, const std::vector<BWAPI::Unit *> & targets) const
	{
		const props::Unit & attackerProps(gameProps.GetUnitProps(attacker));

		BWAPI::Unit *	bestTarget(0);
		int				bestPriorityClass(0);
		float			bestPriority(0);

		foreach(BWAPI::Unit * target, targets)
		{
			// Skip any potential targets we cannot actually attack
			const props::Weapon * ourWeapon(attackerProps.GetCorrespondingWeapon(target));
			if(!ourWeapon)
			{
				continue;
			}

			// Structures for recording desirability of targeting this unit
			int priorityClass(0);
			float priority(0);

			// Determine how many frames it will take us to kill this enemy
			const props::Unit & targetProps(gameProps.GetUnitProps(target));
			float killTime(ourWeapon->GetTimeToKill(targetProps, target->getHitPoints(), target->getShields()));

			// Determine distance to the enemy, and how far we'd have to move to target them
			const float distance(static_cast<float>(attacker->getDistance(const_cast<BWAPI::Unit*>(target))));
			const float ourMoveDistance(std::max(distance - ourWeapon->maxRange, 0.0f));
			if(ourMoveDistance > 0)
			{
				// If we need to move, adjust kill time accordingly
				killTime += ourMoveDistance / attackerProps.moveSpeed;
			}
			else
			{
				// Prefer targets that we can target without moving
				priorityClass += 5;
			}

			// Enemy spellcasters are of highest priority
			const BWAPI::UnitType type(target->getType());
			if(type.isSpellcaster())
			{
				priorityClass += 3;

				// Greatly prefer targets that threaten us
				// Assume spellcaster is dangerous anywhere inside its sight range
				if(distance < targetProps.sightRange)
				{
					priorityClass += 10;
				}
			}
			// Enemies which can target us are of next highest priority
			else if(const props::Weapon * enemyWeapon = targetProps.GetCorrespondingWeapon(attacker))
			{
				priorityClass += 2;

				// Greatly prefer targets that threaten us
				if(distance < enemyWeapon->maxRange)
				{
					priorityClass += 10;
				}

				// Prefer to target enemies which do high damage but can be killed quickly
				const float damageRate(enemyWeapon->GetDamageRateTo(attackerProps.sizeType, attackerProps.hitPointsArmor).damagePerFrame);
				priority = damageRate / killTime;
			}
			// Enemies which can target anything at all are of next highest priority
			else if(const props::Weapon * targetWeapon = targetProps.groundWeapon ? targetProps.groundWeapon : targetProps.airWeapon)
			{
				priorityClass += 1;

				// Prefer to target enemies which do high damage but can be killed quickly
				const float damageRate(static_cast<float>(targetWeapon->damage) / targetWeapon->cooldown);
				priority = damageRate / killTime;
			}
			// Buildings have lowest priority
			else
			{
				// Weights used to determine worth of building
				const float mineralWeight(1.0f);
				const float vespeneWeight(2.0f);
				const float producerBonus(1000.0f);

				// Prefer to target buildings worth the most which can be killed quickly
				float worth(type.mineralPrice() * mineralWeight + type.gasPrice() * vespeneWeight);
				if(type.canProduce()) worth += producerBonus;
				priority = worth / killTime;
			}

			// Unit can be better by being of a higher class, or of a better priority within the original class
			if(!bestTarget || priorityClass > bestPriorityClass || (priorityClass == bestPriorityClass && priority > bestPriority))
			{
				bestTarget			= target;
				bestPriorityClass	= priorityClass;
				bestPriority		= priority;
			}
		}

		return bestTarget;
	}
};

#endif