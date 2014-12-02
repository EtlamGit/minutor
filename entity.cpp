#include "entity.h"

#include "nbt.h"
#include "math.h"

Entity::Entity(double nx, double ny, double nz, QString nid)
	: id(nid)
	, x(nx)
	, y(ny)
	, z(nz)
	, category(Entity::ECAT::ET_OTHER)
{
	defineCategory();
}

Entity::Entity(double nx, double ny, double nz, QString nid, QVariant nproperties)
	: id(nid)
	, x(nx)
	, y(ny)
	, z(nz)
	, category(Entity::ECAT::ET_OTHER)
	, properties(nproperties)
{
	defineCategory();
}


bool Entity::load(Tag *nbt)
{
	Tag* pos = nbt->at("Pos");
	if (!pos) return false;
	x = pos->at(0)->toDouble();
	y = pos->at(1)->toDouble();
	z = pos->at(2)->toDouble();

	Tag* id = nbt->at("id");
	if (!id) return false;
	if (id)
	{
		this->id   = id->toString();
		properties = nbt->getData();
	}
	defineCategory();
	return true;
}


bool Entity::intersects(double x1, double y1, double z1,
                        double x2, double y2, double z2) const
{
    return ((x1 <= this->x) && (this->x <= x2)) &&
           ((y1 <= this->y) && (this->y <= y2)) &&
           ((z1 <= this->z) && (this->z <= z2));
}


void Entity::draw(double offsetX, double offsetZ, double scale, QPainter& canvas) const
{
	double cx = ((x - offsetX) * scale);
	double cz = ((z - offsetZ) * scale);

	QPen pen = canvas.pen();
	pen.setColor(colorP);
	pen.setWidth(2);
	canvas.setPen(pen);

	canvas.setBrush(QBrush(colorB));
	canvas.drawEllipse(QPoint(cx,cz), 5, 5);
}

void Entity::defineCategory()
{
	// define the background color by category
	switch (mobTypes.indexOf(id))
	{
	// hostile
	case  0: // "Blaze"
	case  1: // "CaveSpider"
	case  2: // "Creeper"
	case  3: // "EnderDragon"
	case  4: // "Enderman"
	case  5: // "Endermite"
	case  6: // "Ghast"
	case  7: // "Guardian"
	case  8: // "LavaSlime"
	case  9: // "PigZombie"
	case 10: // "Silverfish"
	case 11: // "Skeleton"
	case 12: // "Slime"
	case 13: // "Spider"
	case 14: // "Witch"
	case 15: // "WitherBoss"
	case 16: // "Zombie"
		colorB = Qt::red;
		category = Entity::ECAT::ET_HOSTILE;
		break;
	// passive
	case 17: // "Bat"
	case 18: // "Chicken"
	case 19: // "Cow"
	case 20: // "EntityHorse"
	case 21: // "Pig"
	case 22: // "Mooshroom"
	case 23: // "Ozelot"
	case 24: // "Rabbit"
	case 25: // "Sheep"
	case 26: // "SnowMan"
	case 27: // "Squid"
	case 28: // "Wolf"
	case 29: // "Villager"
	case 30: // "VillagerGolem"
		colorB = Qt::white;
		category = Entity::ECAT::ET_PASSIVE;
		break;
	// Item
	case 31: // "Item"
		colorB = Qt::blue;
		category = Entity::ECAT::ET_ITEM;
		break;
	// all the rest
	default:
		// generate color from hashed structure name
		quint32 hue = qHash(this->id);
		colorB.setHsv(hue % 360, 255, 255);
		category = Entity::ECAT::ET_OTHER;
	}
	colorB.setAlpha(128);

	//define the outer highlight color by exact Mob type
	switch (mobTypes.indexOf(id))
	{
	// hostile
	case  0: /* "Blaze" */         colorP = Qt::yellow; break;
	case  1: /* "CaveSpider" */    colorP = Qt::darkYellow; break;
	case  2: /* "Creeper" */       colorP = Qt::red; break;
	case  3: /* "EnderDragon" */   colorP = Qt::magenta; break;
	case  4: /* "Enderman" */      colorP = Qt::darkMagenta; break;
	case  5: /* "Endermite" */     colorP = Qt::magenta; break;
	case  6: /* "Ghast" */         colorP = Qt::white; break;
	case  7: /* "Guardian" */      colorP = Qt::darkBlue; break;
	case  8: /* "LavaSlime" */     colorP = Qt::green; break;
	case  9: /* "PigZombie" */     colorP = Qt::darkGreen; break;
	case 10: /* "Silverfish" */    colorP = Qt::darkGray; break;
	case 11: /* "Skeleton" */      colorP = Qt::lightGray; break;
	case 12: /* "Slime" */         colorP = Qt::green; break;
	case 13: /* "Spider" */        colorP = Qt::black; break;
	case 14: /* "Witch" */         colorP = Qt::darkCyan; break;
	case 15: /* "WitherBoss" */    colorP = Qt::darkGray; break;
	case 16: /* "Zombie" */        colorP = Qt::darkGreen; break;

	// passive
	case 17: /* "Bat" */           colorP = Qt::darkRed; break;
	case 18: /* "Chicken" */       colorP = Qt::yellow; break;
	case 19: /* "Cow" */           colorP = Qt::darkCyan; break;
	case 20: /* "EntityHorse" */   colorP = Qt::darkBlue; break;
	case 21: /* "Pig" */           colorP = Qt::magenta; break;
	case 22: /* "Mooshroom" */     colorP = Qt::red; break;
	case 23: /* "Ozelot" */        colorP = Qt::darkYellow; break;
	case 24: /* "Rabbit" */        colorP = Qt::gray; break;
	case 25: /* "Sheep" */         colorP = Qt::lightGray; break;
	case 26: /* "SnowMan" */       colorP = Qt::white; break;
	case 27: /* "Squid" */         colorP = Qt::black; break;
	case 28: /* "Wolf" */          colorP = Qt::darkGray; break;
	case 29: /* "Villager" */ 	   colorP = Qt::darkMagenta; break;
	case 30: /* "VillagerGolem" */ colorP = Qt::cyan; break;

	// Item
	case 31: /* "Item" */          colorP = Qt::white; break;

	// all the rest
	default:
		// generate color from hashed structure name
		quint32 hue = qHash(this->id);
		colorP.setHsv(hue % 360, 255, 255);
	}
	colorP.setAlpha(192);
}


QStringList Entity::mobTypes = QStringList()
	<< "Blaze"         //  0
	<< "CaveSpider"    //  1
	<< "Creeper"       //  2
	<< "EnderDragon"   //  3
	<< "Enderman"      //  4
	<< "Endermite"     //  5
	<< "Ghast"         //  6
	<< "Guardian"      //  7
	<< "LavaSlime"     //  8
	<< "PigZombie"     //  9
	<< "Silverfish"    // 10
	<< "Skeleton"      // 11
	<< "Slime"         // 12
	<< "Spider"        // 13
	<< "Witch"         // 14
	<< "WitherBoss"    // 15
	<< "Zombie"        // 16

	<< "Bat"           // 17
	<< "Chicken"       // 18
	<< "Cow"           // 19
	<< "EntityHorse"   // 20
	<< "Pig"           // 21
	<< "Mooshroom"     // 22
	<< "Ozelot"        // 23
	<< "Rabbit"        // 24
	<< "Sheep"         // 25
	<< "SnowMan"       // 26
	<< "Squid"         // 27
	<< "Wolf"          // 28
	<< "Villager"      // 29
	<< "VillagerGolem" // 30

	<< "Item"          // 31
;
