#include <QtGui>
class GameListing;
class PlayerListing;

#define LESSTHAN	-1
#define EQUALTO		0
#define GREATERTHAN	1

/* ListItem can't have pure virtual functions because
 * we do actually use it for the header items */
class ListItem
{
	public:
		ListItem() {};
		ListItem(const QList <QVariant> & data);
		virtual ~ListItem() {};
		virtual QVariant data(int column) const;
		virtual int columnCount() const;
		virtual int compare(const ListItem &, int) const { return 0;};
	protected:
		int compareNames(const QString & name1, const QString & name2) const;
	private:
		QList <QVariant> itemData;
};

class ObserverListItem : public ListItem
{
	public:
		ObserverListItem(const PlayerListing * l);	
		virtual QVariant data(int column) const;
		int columnCount() const;
		const PlayerListing * getListing(void) const { return listing; };
		virtual int compare(const ListItem &, int column) const;
	private:
		const PlayerListing * listing;
};

class GamesListItem : public ListItem
{
	public:
		GamesListItem(GameListing * const l);
		virtual QVariant data(int column) const;
		int columnCount() const;
		GameListing * getListing(void) const { return listing; };
		virtual int compare(const ListItem &, int column) const;
	private:
		GameListing * const listing;
};

class PlayerListItem : public ListItem
{
	public:
		PlayerListItem(PlayerListing * const l);
		virtual QVariant data(int column) const;
		int columnCount() const;
		PlayerListing * getListing(void) const { return listing; };
		virtual int compare(const ListItem &, int column) const;
	private:
		PlayerListing * const listing;
};

class ListModel : public QAbstractItemModel	//QAbstractItemModel??
{
	 public:
	 	ListModel();
		~ListModel();
		void insertListing(ListItem & item);
		//void updateListing(const ListItem & item);
		//void removeListing(const ListItem & item);
		int priorityCompare(const ListItem & i, const ListItem & j);
		QModelIndex parent(const QModelIndex & index) const;
		//bool hasChildren(const QModelIndex & parent) const;
		int rowCount(const QModelIndex & parent) const;
		int columnCount(const QModelIndex & parent) const;
		//bool hasIndex(int row, int column, const QModelIndex & parent) const;
		QModelIndex index(int row, int column, const QModelIndex & parent) const;
		virtual QVariant data(const QModelIndex & index, int role) const;
		Qt::ItemFlags flags(const QModelIndex & index) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role) const;
		virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
	protected:
		ListItem * headerItem;
		QList <ListItem *> items;	//try it with the one list
		QList <int> sort_priority;
		Qt::SortOrder list_sort_order;
	private:
		void quicksort(int b, int e);
		int qs_partition(int b, int e);
};

class ObserverListModel : public ListModel
{
	public:
		ObserverListModel();
		~ObserverListModel();
		void insertListing(const PlayerListing * l);
		void removeListing(const PlayerListing * l);
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
		void setAccountName(QString name) { account_name = name; };
	private:
		QString account_name;
};

class PlayerListModel : public ListModel
{
	public:
		PlayerListModel();
		~PlayerListModel();
		void insertListing(PlayerListing * const l);
		void updateListing(PlayerListing * const l);
		void removeListing(PlayerListing * const l);
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
		PlayerListing * playerListingFromIndex(const QModelIndex &);
		void setAccountName(QString name) { account_name = name; };
	private:
		QString account_name;
};

class SimplePlayerListModel: public PlayerListModel
{
	public:
		SimplePlayerListModel(bool _notify_column);
		virtual QVariant data(const QModelIndex & index, int role) const;
	private:
		bool notify_column;
};

class GamesListModel : public ListModel
{
	public:
		GamesListModel();
		~GamesListModel();
		void insertListing(GameListing * const l);
		void updateListing(GameListing * const l);
		void removeListing(GameListing * const l);
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
		GameListing * gameListingFromIndex(const QModelIndex &);
};

class PlayerSortProxy : public QSortFilterProxyModel
{
	/* IGS ranks probably up to 10000, but ORO would need 100000 so... */
	public:
		PlayerSortProxy() : rankMin(0), rankMax(100000), flags(none) {};
		virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
		bool filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const;
		void setFilter(int rn, int rm);
		enum PlayerSortFlags { none = 0x0, open = 0x1, friends = 0x2, fans = 0x4, noblock = 0x8 };
		void setFilter(enum PlayerSortFlags f);
	private:
		int rankMin;
		int rankMax;
		unsigned char flags;
};

extern std::map<class PlayerListing *, unsigned short> removed_player;
