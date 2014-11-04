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

//	QPen pen = canvas.pen();
//	pen.setColor(color);
	//pen.setWidth(MIN_SIZE/2);
	canvas.setPen(QColor(0, 0, 0, 0));

	canvas.setBrush(QBrush(color));
	canvas.drawEllipse(QPoint(cx,cz), 5, 5);
}

void Entity::defineCategory()
{
	if ( (id=="Blaze") ||
		 (id=="CaveSpider") ||
		 (id=="Creeper") ||
		 (id=="EnderDragon") ||
		 (id=="Enderman") ||
		 (id=="Endermite") ||
		 (id=="Ghast") ||
		 (id=="Guardian") ||
		 (id=="LavaSlime") ||
		 (id=="PigZombie") ||
		 (id=="Silverfish") ||
		 (id=="Skeleton") ||
		 (id=="Slime") ||
		 (id=="Spider") ||
		 (id=="Witch") ||
		 (id=="WitherBoss") ||
		 (id=="Zombie") )
	{
		// hostile
		color = Qt::red;
		category = Entity::ECAT::ET_HOSTILE; 
	} else if ( (id=="Bat") ||
				(id=="Chicken") ||
				(id=="Cow") ||
				(id=="EntityHorse") ||
				(id=="Pig") ||
				(id=="Mooshroom") ||
				(id=="Ocelot") ||
				(id=="Rabbit") ||
				(id=="Sheep") ||
				(id=="SnowMan") ||
				(id=="Squid") ||
				(id=="Wolf") ||
				(id=="Villager") ||
				(id=="VillagerGolem") )
	{
		// passive
		color = Qt::white;
		category = Entity::ECAT::ET_PASSIVE; 
	} else if (id=="Item")
	{
		// Item
		color = Qt::blue;
		category = Entity::ECAT::ET_ITEM; 
	}
	else
	{
		// generate color from hashed structure name	
		quint32 hue = qHash(this->id);
		color.setHsv(hue % 360, 255, 255);
		category = Entity::ECAT::ET_OTHER; 
	}
	color.setAlpha(128);
}

